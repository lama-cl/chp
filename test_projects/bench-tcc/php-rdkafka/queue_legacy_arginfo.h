/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 9e80d48bb60ede4003fffcfe0da09ac0e5c2f4d1 */

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RdKafka_Queue___construct, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RdKafka_Queue_consume, 0, 0, 1)
	ZEND_ARG_INFO(0, timeout_ms)
ZEND_END_ARG_INFO()


ZEND_METHOD(RdKafka, __construct);
ZEND_METHOD(RdKafka_Queue, consume);


static const zend_function_entry class_RdKafka_Queue_methods[] = {
	ZEND_MALIAS(RdKafka, __construct, __construct, arginfo_class_RdKafka_Queue___construct, ZEND_ACC_PRIVATE)
	ZEND_ME(RdKafka_Queue, consume, arginfo_class_RdKafka_Queue_consume, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static zend_class_entry *register_class_RdKafka_Queue(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "RdKafka", "Queue", class_RdKafka_Queue_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);

	return class_entry;
}
