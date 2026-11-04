#ifndef VSF_SYSSTR_H
#define VSF_SYSSTR_H

/* Forward declarations */
struct mystr;
struct vsf_sysutil_statbuf;
struct vsf_sysutil_dir;
struct vsf_sysutil_user;

void str_getcwd(struct mystr *p_str : itype(_Ptr<struct mystr>));
int str_readlink(struct mystr *p_str : itype(_Ptr<struct mystr>), const struct mystr *p_filename_str : itype(_Ptr<const struct mystr>));
int str_write_loop(const struct mystr *p_str : itype(_Ptr<const struct mystr>), const int fd);
int str_read_loop(struct mystr *p_str : itype(_Ptr<struct mystr>), const int fd);
int str_mkdir(const struct mystr *p_str : itype(_Ptr<const struct mystr>), const unsigned int mode);
int str_rmdir(const struct mystr *p_str : itype(_Ptr<const struct mystr>));
int str_unlink(const struct mystr *p_str : itype(_Ptr<const struct mystr>));
int str_chdir(const struct mystr *p_str : itype(_Ptr<const struct mystr>));
enum EVSFSysStrOpenMode
{
  kVSFSysStrOpenUnknown = 0,
  kVSFSysStrOpenReadOnly = 1
};
int str_open(const struct mystr *p_str : itype(_Ptr<const struct mystr>), const enum EVSFSysStrOpenMode mode);
int str_create(const struct mystr *p_str : itype(_Ptr<const struct mystr>));
int str_create_exclusive(const struct mystr *p_str : itype(_Ptr<const struct mystr>));
int str_chmod(const struct mystr *p_str : itype(_Ptr<const struct mystr>), unsigned int mode);
int str_stat(const struct mystr *p_str : itype(_Ptr<const struct mystr>), struct vsf_sysutil_statbuf **p_ptr : itype(_Ptr<_Ptr<struct vsf_sysutil_statbuf>>));
int str_lstat(const struct mystr *p_str : itype(_Ptr<const struct mystr>), struct vsf_sysutil_statbuf **p_ptr : itype(_Ptr<_Ptr<struct vsf_sysutil_statbuf>>));
int str_rename(const struct mystr *p_from_str : itype(_Ptr<const struct mystr>), const struct mystr *p_to_str : itype(_Ptr<const struct mystr>));
struct vsf_sysutil_dir *str_opendir(const struct mystr *p_str : itype(_Ptr<const struct mystr>)) : itype(_Ptr<struct vsf_sysutil_dir>);
void str_next_dirent(struct mystr *p_filename_str : itype(_Ptr<struct mystr>), struct vsf_sysutil_dir *p_dir : itype(_Ptr<struct vsf_sysutil_dir>));

struct vsf_sysutil_user *str_getpwnam(const struct mystr *p_user_str : itype(_Ptr<const struct mystr>)) : itype(_Ptr<struct vsf_sysutil_user>);

void str_syslog(const struct mystr *p_str : itype(_Ptr<const struct mystr>), int severe);

#endif /* VSF_SYSSTR_H */

