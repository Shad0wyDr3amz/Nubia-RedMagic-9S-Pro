// <copyright file="config_check.c" company="Paragon Software Group">
// EXCEPT WHERE OTHERWISE STATED, THE INFORMATION AND SOURCE CODE CONTAINED
// HEREIN AND IN RELATED FILES IS THE EXCLUSIVE PROPERTY OF PARAGON SOFTWARE
// GROUP COMPANY AND MAY NOT BE EXAMINED, DISTRIBUTED, DISCLOSED, OR REPRODUCED
// IN WHOLE OR IN PART WITHOUT EXPLICIT WRITTEN AUTHORIZATION FROM THE COMPANY.
//
// Copyright (c) 1994-2017 Paragon Software Group, All rights reserved.
//
// UNLESS OTHERWISE AGREED IN A WRITING SIGNED BY THE PARTIES, THIS SOFTWARE IS
// PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE, ALL OF WHICH ARE HEREBY DISCLAIMED. IN NO EVENT SHALL THE
// AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF NOT ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
// </copyright>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/zlib.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

#include "ufsdapi.h"

#define UFSD_HASH_RES_NOTCHECKED  0xffffffffL

char ufsd_hash_check_result[64] = { 0 };

#if ((defined CONFIG_IKCONFIG_PROC) && (defined CONFIG_ZLIB_INFLATE))

static size_t
__init ufsd_get_file_size(struct file* filp)
{
  mm_segment_t oldfs;
  size_t size = -EINVAL;

  oldfs = get_fs();
  set_fs(KERNEL_DS);

  size = vfs_llseek( filp, 0L, SEEK_END );
  if ( size > 0 ) {
    vfs_llseek( filp, 0L, SEEK_SET );
  }

  set_fs(oldfs);

  return size;
}

static void
__init ufsd_make_crc_table(
    IN unsigned long *crc_table
    )
{
  unsigned long c;
  int n, k;

  if(!crc_table)
    return;

  for (n = 0; n < 256; ++n) {
    c = (unsigned long) n;
    for (k = 0; k < 8; ++k) {
      if (c & 1)
        c = 0xedb88320L ^ (c >> 1);
      else
        c = c >> 1;
    }
    crc_table[n] = c;
  }
}

static unsigned long
__init ufsd_update_crc(
    IN const unsigned long crc,
    IN const unsigned char * const buf,
    IN int len,
    IN unsigned long *crc_table)
{
  unsigned long c = crc;
  int n;

  for (n = 0; n < len; ++n) {
    c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
  }
  return c;
}

unsigned long
__init ufsd_calc_config_hash(void)
{
  int ret = -1;

  #define IN_SIZE 1024
  unsigned char *in = NULL;

  #define OUT_SIZE 1024
  unsigned char *out = NULL;

  struct file* filp = NULL;
  mm_segment_t oldfs;
  const char *path = "/proc/config.gz";
  unsigned long long offset = 10; // to skip gzip header
  size_t fsize = 0;

  z_stream *zs = NULL;

  unsigned long *crc_table=NULL;
  unsigned long crc  = UFSD_HASH_RES_NOTCHECKED;

  oldfs = get_fs();
  set_fs(KERNEL_DS);
  filp = filp_open(path, O_RDONLY, 0);
  set_fs(oldfs);

  if (IS_ERR(filp)) {
    printk( KERN_WARNING QUOTED_UFSD_DEVICE": Can't open /proc/config.gz\n");
    return crc;
  }

  in = kmalloc(IN_SIZE, GFP_KERNEL);
  out = kmalloc(OUT_SIZE, GFP_KERNEL);
  zs = kmalloc(sizeof(*zs), GFP_KERNEL);
  crc_table = kmalloc(sizeof(unsigned long)*256, GFP_KERNEL);
  if (!in || !out || !zs || !crc_table) {
    printk( KERN_WARNING QUOTED_UFSD_DEVICE": Can't alloc buffers.\n");
    goto memfree;
  }

  zs->workspace = kmalloc(zlib_inflate_workspacesize(), GFP_KERNEL);
  if (!zs->workspace) {
    printk( KERN_WARNING QUOTED_UFSD_DEVICE": Can't alloc buffer.\n");
    goto memfree;
  }

  ufsd_make_crc_table(crc_table);

  zs->next_in   = NULL;
  zs->avail_in  = 0;
  zs->next_out  = out;
  zs->avail_out = OUT_SIZE - 1;
  ret = zlib_inflateInit2(zs, -MAX_WBITS);
  if (ret != Z_OK) {
    printk( KERN_WARNING QUOTED_UFSD_DEVICE": Can't init zlib.\n");
    goto memfree;
  }

  fsize = ufsd_get_file_size(filp);
  if (fsize == -EINVAL) {
    printk( KERN_WARNING QUOTED_UFSD_DEVICE": Can't get file size.\n");
    goto memfree;
  }

  zs->next_in   = in;
  while (offset<fsize) {
    oldfs = get_fs();
    set_fs(get_ds());
    ret = vfs_read(filp, in, IN_SIZE, &offset);
    set_fs(oldfs);

    if (ret < 1) {
      printk( KERN_WARNING QUOTED_UFSD_DEVICE": Can't read file.\n");
      crc = UFSD_HASH_RES_NOTCHECKED;
      break;
    }

    zs->next_in  = in;
    zs->avail_in = ret;

    do {
      zs->next_out  = out;
      zs->avail_out = OUT_SIZE - 1;

      ret = zlib_inflate(zs, Z_SYNC_FLUSH);

      if (ret != Z_OK && ret != Z_BUF_ERROR && ret != Z_STREAM_END) {
        printk( KERN_WARNING QUOTED_UFSD_DEVICE": zlib inflate error %i.\n", ret);
        crc = UFSD_HASH_RES_NOTCHECKED;
        break;
      } else {
        crc = ufsd_update_crc(crc, out, (OUT_SIZE - 1 - zs->avail_out), crc_table);

        if(ret == Z_STREAM_END) break; // end of stream
      }
    }
    while (zs->avail_in > 0);
  }

  if (crc != UFSD_HASH_RES_NOTCHECKED ) {
    crc ^= 0xffffffffL;
  } else {
    printk( KERN_NOTICE QUOTED_UFSD_DEVICE": CRC calculation failed.\n");
  }

  zlib_inflateEnd(zs);

memfree:
  if (zs)  {
    kfree(zs->workspace);
    kfree(zs);
  }
  kfree(out);
  kfree(in);
  kfree(crc_table);

  filp_close(filp, NULL);

  return crc;
}

#endif

void
__init ufsd_check_config_hash(void)
{
  unsigned long crc  = UFSD_HASH_RES_NOTCHECKED;

#if ((defined CONFIG_IKCONFIG_PROC) && (defined CONFIG_ZLIB_INFLATE))
  crc = ufsd_calc_config_hash();
#endif

#ifdef UFSD_KCONFIG_HASH_VAL
  if ( crc == UFSD_KCONFIG_HASH_VAL ) {
    snprintf(ufsd_hash_check_result, sizeof(ufsd_hash_check_result),
             "%s",
             "original "__stringify(UFSD_KCONFIG_HASH_VAL)", current matched");
  } else if ( crc == 0xffffffffL ){
    snprintf(ufsd_hash_check_result, sizeof(ufsd_hash_check_result),
             "%s",
             "original "__stringify(UFSD_KCONFIG_HASH_VAL)", current can't check");
  } else {
    snprintf(ufsd_hash_check_result, sizeof(ufsd_hash_check_result),
             "original "__stringify(UFSD_KCONFIG_HASH_VAL)
             ", current %#08lx, not matched", crc);
  }
#else
  if ( crc == 0xffffffffL ){
    snprintf(ufsd_hash_check_result, sizeof(ufsd_hash_check_result),
             "%s",
             "original unknown, current can't check");
  } else {
    snprintf(ufsd_hash_check_result, sizeof(ufsd_hash_check_result),
             "original unknown, current %#08lx can't check", crc);
  }
#endif

  printk( KERN_NOTICE QUOTED_UFSD_DEVICE": Kernel .config hash: %s.\n",
          ufsd_hash_check_result);
}

