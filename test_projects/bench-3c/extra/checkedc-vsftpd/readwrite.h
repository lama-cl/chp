#ifndef VSF_READWRITE_H
#define VSF_READWRITE_H

struct vsf_session;
struct mystr;

enum EVSFRWTarget
{
  kVSFRWControl = 1,
  kVSFRWData
};

int ftp_write_str(const struct vsf_session *p_sess : itype(_Ptr<const struct vsf_session>), const struct mystr *p_str : itype(_Ptr<const struct mystr>), enum EVSFRWTarget target);
int ftp_read_data(struct vsf_session *p_sess : itype(_Ptr<struct vsf_session>), char *p_buf : itype(_Array_ptr<char>) byte_count(len), unsigned int len);
int ftp_write_data(const struct vsf_session *p_sess : itype(_Ptr<const struct vsf_session>), const char *p_buf : itype(_Array_ptr<const char>) byte_count(len), unsigned int len);
int ftp_getline(struct vsf_session *p_sess : itype(_Ptr<struct vsf_session>), struct mystr *p_str : itype(_Ptr<struct mystr>), char *p_buf : itype(_Array_ptr<char>) count(4096));

#endif /* VSF_READWRITE_H */

