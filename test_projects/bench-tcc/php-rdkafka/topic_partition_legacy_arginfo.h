/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 7c722b9eb9357157d89a14431ebcfd79cc6f1116 */

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RdKafka_TopicPartition___construct, 0, 0, 2)
	ZEND_ARG_INFO(0, topic)
	ZEND_ARG_INFO(0, partition)
	ZEND_ARG_INFO(0, offset)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RdKafka_TopicPartition_getTopic, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RdKafka_TopicPartition_setTopic, 0, 0, 1)
	ZEND_ARG_INFO(0, topic_name)
ZEND_END_ARG_INFO()

#define arginfo_class_RdKafka_TopicPartition_getPartition arginfo_class_RdKafka_TopicPartition_getTopic

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RdKafka_TopicPartition_setPartition, 0, 0, 1)
	ZEND_ARG_INFO(0, partition)
ZEND_END_ARG_INFO()

#define arginfo_class_RdKafka_TopicPartition_getOffset arginfo_class_RdKafka_TopicPartition_getTopic

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RdKafka_TopicPartition_setOffset, 0, 0, 1)
	ZEND_ARG_INFO(0, offset)
ZEND_END_ARG_INFO()

#define arginfo_class_RdKafka_TopicPartition_getErr arginfo_class_RdKafka_TopicPartition_getTopic


ZEND_METHOD(RdKafka_TopicPartition, __construct);
ZEND_METHOD(RdKafka_TopicPartition, getTopic);
ZEND_METHOD(RdKafka_TopicPartition, setTopic);
ZEND_METHOD(RdKafka_TopicPartition, getPartition);
ZEND_METHOD(RdKafka_TopicPartition, setPartition);
ZEND_METHOD(RdKafka_TopicPartition, getOffset);
ZEND_METHOD(RdKafka_TopicPartition, setOffset);
ZEND_METHOD(RdKafka_TopicPartition, getErr);


static const zend_function_entry class_RdKafka_TopicPartition_methods[] = {
	ZEND_ME(RdKafka_TopicPartition, __construct, arginfo_class_RdKafka_TopicPartition___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_TopicPartition, getTopic, arginfo_class_RdKafka_TopicPartition_getTopic, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_TopicPartition, setTopic, arginfo_class_RdKafka_TopicPartition_setTopic, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_TopicPartition, getPartition, arginfo_class_RdKafka_TopicPartition_getPartition, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_TopicPartition, setPartition, arginfo_class_RdKafka_TopicPartition_setPartition, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_TopicPartition, getOffset, arginfo_class_RdKafka_TopicPartition_getOffset, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_TopicPartition, setOffset, arginfo_class_RdKafka_TopicPartition_setOffset, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_TopicPartition, getErr, arginfo_class_RdKafka_TopicPartition_getErr, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static zend_class_entry *register_class_RdKafka_TopicPartition(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "RdKafka", "TopicPartition", class_RdKafka_TopicPartition_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);

	return class_entry;
}
