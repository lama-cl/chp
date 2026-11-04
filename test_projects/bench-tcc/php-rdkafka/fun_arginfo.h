/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 0e1e56d853a47168a1f7f0950b674c2de6a91976 */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rd_kafka_get_err_descs, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rd_kafka_err2name, 0, 1, IS_STRING, 1)
	ZEND_ARG_TYPE_INFO(0, err, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_rd_kafka_err2str arginfo_rd_kafka_err2name

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rd_kafka_errno2err, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, errnox, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rd_kafka_errno, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rd_kafka_offset_tail, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, cnt, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_rd_kafka_thread_cnt arginfo_rd_kafka_errno


ZEND_FUNCTION(rd_kafka_get_err_descs);
ZEND_FUNCTION(rd_kafka_err2name);
ZEND_FUNCTION(rd_kafka_err2str);
ZEND_FUNCTION(rd_kafka_errno2err);
ZEND_FUNCTION(rd_kafka_errno);
ZEND_FUNCTION(rd_kafka_offset_tail);
ZEND_FUNCTION(rd_kafka_thread_cnt);


static const zend_function_entry ext_functions[] = {
	ZEND_FE(rd_kafka_get_err_descs, arginfo_rd_kafka_get_err_descs)
	ZEND_FE(rd_kafka_err2name, arginfo_rd_kafka_err2name)
	ZEND_FE(rd_kafka_err2str, arginfo_rd_kafka_err2str)
	ZEND_DEP_FE(rd_kafka_errno2err, arginfo_rd_kafka_errno2err)
	ZEND_DEP_FE(rd_kafka_errno, arginfo_rd_kafka_errno)
	ZEND_FE(rd_kafka_offset_tail, arginfo_rd_kafka_offset_tail)
	ZEND_FE(rd_kafka_thread_cnt, arginfo_rd_kafka_thread_cnt)
	ZEND_FE_END
};
