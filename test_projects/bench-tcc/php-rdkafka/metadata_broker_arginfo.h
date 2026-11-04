/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 74c6ee55c31bb86f5bcf71a46607f31688ce71dd */

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RdKafka_Metadata_Broker___construct, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_RdKafka_Metadata_Broker_getId, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_RdKafka_Metadata_Broker_getHost, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RdKafka_Metadata_Broker_getPort arginfo_class_RdKafka_Metadata_Broker_getId


ZEND_METHOD(RdKafka, __construct);
ZEND_METHOD(RdKafka_Metadata_Broker, getId);
ZEND_METHOD(RdKafka_Metadata_Broker, getHost);
ZEND_METHOD(RdKafka_Metadata_Broker, getPort);


static const zend_function_entry class_RdKafka_Metadata_Broker_methods[] = {
	ZEND_MALIAS(RdKafka, __construct, __construct, arginfo_class_RdKafka_Metadata_Broker___construct, ZEND_ACC_PRIVATE)
	ZEND_ME(RdKafka_Metadata_Broker, getId, arginfo_class_RdKafka_Metadata_Broker_getId, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_Metadata_Broker, getHost, arginfo_class_RdKafka_Metadata_Broker_getHost, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_Metadata_Broker, getPort, arginfo_class_RdKafka_Metadata_Broker_getPort, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static zend_class_entry *register_class_RdKafka_Metadata_Broker(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "RdKafka\\Metadata", "Broker", class_RdKafka_Metadata_Broker_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);

	return class_entry;
}
