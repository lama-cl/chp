#ifndef VSF_SSL_H
#define VSF_SSL_H

struct vsf_session;
struct mystr;

int ssl_read(struct vsf_session *p_sess : itype(_Ptr<struct vsf_session>), void* p_ssl : itype(_Ptr<void>), char *p_buf : itype(_Array_ptr<char>) count(len), unsigned int len);
int ssl_peek(struct vsf_session *p_sess : itype(_Ptr<struct vsf_session>), void* p_ssl : itype(_Ptr<void>), char *p_buf : itype(_Array_ptr<char>) count(len), unsigned int len);
int ssl_write(void* p_ssl : itype(_Ptr<void>), const char *p_buf : itype(_Array_ptr<const char>) count(len), unsigned int len);
int ssl_write_str(void* p_ssl : itype(_Ptr<void>), const struct mystr *p_str : itype(_Ptr<const struct mystr>));
int ssl_read_into_str(struct vsf_session *p_sess : itype(_Ptr<struct vsf_session>), void* p_ssl : itype(_Ptr<void>), struct mystr *p_str : itype(_Ptr<struct mystr>));
void ssl_init(struct vsf_session *p_sess : itype(_Ptr<struct vsf_session>));
int ssl_accept(struct vsf_session *p_sess : itype(_Ptr<struct vsf_session>), int fd);
int ssl_data_close(struct vsf_session *p_sess : itype(_Ptr<struct vsf_session>));
void ssl_comm_channel_init(struct vsf_session *p_sess : itype(_Ptr<struct vsf_session>));
void ssl_comm_channel_set_consumer_context(struct vsf_session *p_sess : itype(_Ptr<struct vsf_session>));
void ssl_comm_channel_set_producer_context(struct vsf_session *p_sess : itype(_Ptr<struct vsf_session>));
void handle_auth(struct vsf_session *p_sess : itype(_Ptr<struct vsf_session>));
void handle_pbsz(struct vsf_session *p_sess : itype(_Ptr<struct vsf_session>));
void handle_prot(struct vsf_session *p_sess : itype(_Ptr<struct vsf_session>));
void ssl_control_handshake(struct vsf_session *p_sess : itype(_Ptr<struct vsf_session>));
void ssl_add_entropy(struct vsf_session *p_sess : itype(_Ptr<struct vsf_session>));

#endif /* VSF_SSL_H */

