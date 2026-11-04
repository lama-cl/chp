/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 1a50cd552973f23b01a2d6b4e5464ba14320c393 */

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RdKafka_KafkaErrorException___construct, 0, 0, 6)
	ZEND_ARG_INFO(0, message)
	ZEND_ARG_INFO(0, code)
	ZEND_ARG_INFO(0, error_string)
	ZEND_ARG_INFO(0, isFatal)
	ZEND_ARG_INFO(0, isRetriable)
	ZEND_ARG_INFO(0, transactionRequiresAbort)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RdKafka_KafkaErrorException_getErrorString, 0, 0, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RdKafka_KafkaErrorException_isFatal arginfo_class_RdKafka_KafkaErrorException_getErrorString

#define arginfo_class_RdKafka_KafkaErrorException_isRetriable arginfo_class_RdKafka_KafkaErrorException_getErrorString

#define arginfo_class_RdKafka_KafkaErrorException_transactionRequiresAbort arginfo_class_RdKafka_KafkaErrorException_getErrorString


ZEND_METHOD(RdKafka_KafkaErrorException, __construct);
ZEND_METHOD(RdKafka_KafkaErrorException, getErrorString);
ZEND_METHOD(RdKafka_KafkaErrorException, isFatal);
ZEND_METHOD(RdKafka_KafkaErrorException, isRetriable);
ZEND_METHOD(RdKafka_KafkaErrorException, transactionRequiresAbort);


static const zend_function_entry class_RdKafka_KafkaErrorException_methods[] = {
	ZEND_ME(RdKafka_KafkaErrorException, __construct, arginfo_class_RdKafka_KafkaErrorException___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_KafkaErrorException, getErrorString, arginfo_class_RdKafka_KafkaErrorException_getErrorString, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_KafkaErrorException, isFatal, arginfo_class_RdKafka_KafkaErrorException_isFatal, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_KafkaErrorException, isRetriable, arginfo_class_RdKafka_KafkaErrorException_isRetriable, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_KafkaErrorException, transactionRequiresAbort, arginfo_class_RdKafka_KafkaErrorException_transactionRequiresAbort, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static zend_class_entry *register_class_RdKafka_KafkaErrorException(zend_class_entry *class_entry_RdKafka_Exception)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "RdKafka", "KafkaErrorException", class_RdKafka_KafkaErrorException_methods);
	class_entry = zend_register_internal_class_ex(&ce, class_entry_RdKafka_Exception);

	zval property_error_string_default_value;
	ZVAL_NULL(&property_error_string_default_value);
	zend_string *property_error_string_name = zend_string_init("error_string", sizeof("error_string") - 1, 1);
	zend_declare_property_ex(class_entry, property_error_string_name, &property_error_string_default_value, ZEND_ACC_PRIVATE, NULL);
	zend_string_release(property_error_string_name);

	zval property_isFatal_default_value;
	ZVAL_NULL(&property_isFatal_default_value);
	zend_string *property_isFatal_name = zend_string_init("isFatal", sizeof("isFatal") - 1, 1);
	zend_declare_property_ex(class_entry, property_isFatal_name, &property_isFatal_default_value, ZEND_ACC_PRIVATE, NULL);
	zend_string_release(property_isFatal_name);

	zval property_isRetriable_default_value;
	ZVAL_NULL(&property_isRetriable_default_value);
	zend_string *property_isRetriable_name = zend_string_init("isRetriable", sizeof("isRetriable") - 1, 1);
	zend_declare_property_ex(class_entry, property_isRetriable_name, &property_isRetriable_default_value, ZEND_ACC_PRIVATE, NULL);
	zend_string_release(property_isRetriable_name);

	zval property_transactionRequiresAbort_default_value;
	ZVAL_NULL(&property_transactionRequiresAbort_default_value);
	zend_string *property_transactionRequiresAbort_name = zend_string_init("transactionRequiresAbort", sizeof("transactionRequiresAbort") - 1, 1);
	zend_declare_property_ex(class_entry, property_transactionRequiresAbort_name, &property_transactionRequiresAbort_default_value, ZEND_ACC_PRIVATE, NULL);
	zend_string_release(property_transactionRequiresAbort_name);

	return class_entry;
}
