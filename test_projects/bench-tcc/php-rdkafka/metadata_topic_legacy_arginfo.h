/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 04fd71ae954bd0a09730d401c8160574e1045369 */

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RdKafka_Metadata_Topic___construct, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RdKafka_Metadata_Topic_getTopic, 0, 0, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RdKafka_Metadata_Topic_getErr arginfo_class_RdKafka_Metadata_Topic_getTopic

#define arginfo_class_RdKafka_Metadata_Topic_getPartitions arginfo_class_RdKafka_Metadata_Topic_getTopic


ZEND_METHOD(RdKafka, __construct);
ZEND_METHOD(RdKafka_Metadata_Topic, getTopic);
ZEND_METHOD(RdKafka_Metadata_Topic, getErr);
ZEND_METHOD(RdKafka_Metadata_Topic, getPartitions);


static const zend_function_entry class_RdKafka_Metadata_Topic_methods[] = {
	ZEND_MALIAS(RdKafka, __construct, __construct, arginfo_class_RdKafka_Metadata_Topic___construct, ZEND_ACC_PRIVATE)
	ZEND_ME(RdKafka_Metadata_Topic, getTopic, arginfo_class_RdKafka_Metadata_Topic_getTopic, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_Metadata_Topic, getErr, arginfo_class_RdKafka_Metadata_Topic_getErr, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_Metadata_Topic, getPartitions, arginfo_class_RdKafka_Metadata_Topic_getPartitions, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static zend_class_entry *register_class_RdKafka_Metadata_Topic(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "RdKafka\\Metadata", "Topic", class_RdKafka_Metadata_Topic_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);

	return class_entry;
}
