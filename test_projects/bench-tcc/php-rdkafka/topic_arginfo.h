/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 7d23f208609b509bb75f8286fc5c93c127c406d6 */

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_RdKafka_Topic_getName, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RdKafka_ConsumerTopic___construct, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_RdKafka_ConsumerTopic_consumeQueueStart, 0, 3, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, partition, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, offset, IS_LONG, 0)
	ZEND_ARG_OBJ_INFO(0, queue, RdKafka\\Queue, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_RdKafka_ConsumerTopic_consumeCallback, 0, 3, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, partition, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, timeout_ms, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, callback, IS_CALLABLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_RdKafka_ConsumerTopic_consumeStart, 0, 2, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, partition, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, offset, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_RdKafka_ConsumerTopic_consumeStop, 0, 1, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, partition, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_OBJ_INFO_EX(arginfo_class_RdKafka_ConsumerTopic_consume, 0, 2, RdKafka\\Message, 1)
	ZEND_ARG_TYPE_INFO(0, partition, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, timeout_ms, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_RdKafka_ConsumerTopic_consumeBatch, 0, 3, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, partition, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, timeout_ms, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, batch_size, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RdKafka_ConsumerTopic_offsetStore arginfo_class_RdKafka_ConsumerTopic_consumeStart

#define arginfo_class_RdKafka_KafkaConsumerTopic___construct arginfo_class_RdKafka_ConsumerTopic___construct

#define arginfo_class_RdKafka_KafkaConsumerTopic_offsetStore arginfo_class_RdKafka_ConsumerTopic_consumeStart

#define arginfo_class_RdKafka_ProducerTopic___construct arginfo_class_RdKafka_ConsumerTopic___construct

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_RdKafka_ProducerTopic_produce, 0, 2, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, partition, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, msgflags, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, payload, IS_STRING, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, key, IS_STRING, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, msg_opaque, IS_STRING, 1, "null")
ZEND_END_ARG_INFO()

#if defined(HAVE_RD_KAFKA_MESSAGE_HEADERS)
ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_RdKafka_ProducerTopic_producev, 0, 2, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, partition, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, msgflags, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, payload, IS_STRING, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, key, IS_STRING, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, headers, IS_ARRAY, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, timestamp_ms, IS_LONG, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, msg_opaque, IS_STRING, 1, "null")
ZEND_END_ARG_INFO()
#endif


ZEND_METHOD(RdKafka_Topic, getName);
ZEND_METHOD(RdKafka, __construct);
ZEND_METHOD(RdKafka_ConsumerTopic, consumeQueueStart);
ZEND_METHOD(RdKafka_ConsumerTopic, consumeCallback);
ZEND_METHOD(RdKafka_ConsumerTopic, consumeStart);
ZEND_METHOD(RdKafka_ConsumerTopic, consumeStop);
ZEND_METHOD(RdKafka_ConsumerTopic, consume);
ZEND_METHOD(RdKafka_ConsumerTopic, consumeBatch);
ZEND_METHOD(RdKafka_ConsumerTopic, offsetStore);
ZEND_METHOD(RdKafka_ProducerTopic, produce);
#if defined(HAVE_RD_KAFKA_MESSAGE_HEADERS)
ZEND_METHOD(RdKafka_ProducerTopic, producev);
#endif


static const zend_function_entry class_RdKafka_Topic_methods[] = {
	ZEND_ME(RdKafka_Topic, getName, arginfo_class_RdKafka_Topic_getName, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};


static const zend_function_entry class_RdKafka_ConsumerTopic_methods[] = {
	ZEND_MALIAS(RdKafka, __construct, __construct, arginfo_class_RdKafka_ConsumerTopic___construct, ZEND_ACC_PRIVATE)
	ZEND_ME(RdKafka_ConsumerTopic, consumeQueueStart, arginfo_class_RdKafka_ConsumerTopic_consumeQueueStart, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_ConsumerTopic, consumeCallback, arginfo_class_RdKafka_ConsumerTopic_consumeCallback, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_ConsumerTopic, consumeStart, arginfo_class_RdKafka_ConsumerTopic_consumeStart, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_ConsumerTopic, consumeStop, arginfo_class_RdKafka_ConsumerTopic_consumeStop, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_ConsumerTopic, consume, arginfo_class_RdKafka_ConsumerTopic_consume, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_ConsumerTopic, consumeBatch, arginfo_class_RdKafka_ConsumerTopic_consumeBatch, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_ConsumerTopic, offsetStore, arginfo_class_RdKafka_ConsumerTopic_offsetStore, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};


static const zend_function_entry class_RdKafka_KafkaConsumerTopic_methods[] = {
	ZEND_MALIAS(RdKafka, __construct, __construct, arginfo_class_RdKafka_KafkaConsumerTopic___construct, ZEND_ACC_PRIVATE)
	ZEND_MALIAS(RdKafka_ConsumerTopic, offsetStore, offsetStore, arginfo_class_RdKafka_KafkaConsumerTopic_offsetStore, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};


static const zend_function_entry class_RdKafka_ProducerTopic_methods[] = {
	ZEND_MALIAS(RdKafka, __construct, __construct, arginfo_class_RdKafka_ProducerTopic___construct, ZEND_ACC_PRIVATE)
	ZEND_ME(RdKafka_ProducerTopic, produce, arginfo_class_RdKafka_ProducerTopic_produce, ZEND_ACC_PUBLIC)
#if defined(HAVE_RD_KAFKA_MESSAGE_HEADERS)
	ZEND_ME(RdKafka_ProducerTopic, producev, arginfo_class_RdKafka_ProducerTopic_producev, ZEND_ACC_PUBLIC)
#endif
	ZEND_FE_END
};

static zend_class_entry *register_class_RdKafka_Topic(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "RdKafka", "Topic", class_RdKafka_Topic_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	class_entry->ce_flags |= ZEND_ACC_ABSTRACT;

	return class_entry;
}

static zend_class_entry *register_class_RdKafka_ConsumerTopic(zend_class_entry *class_entry_RdKafka_Topic)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "RdKafka", "ConsumerTopic", class_RdKafka_ConsumerTopic_methods);
	class_entry = zend_register_internal_class_ex(&ce, class_entry_RdKafka_Topic);

	return class_entry;
}

static zend_class_entry *register_class_RdKafka_KafkaConsumerTopic(zend_class_entry *class_entry_RdKafka_Topic)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "RdKafka", "KafkaConsumerTopic", class_RdKafka_KafkaConsumerTopic_methods);
	class_entry = zend_register_internal_class_ex(&ce, class_entry_RdKafka_Topic);

	return class_entry;
}

static zend_class_entry *register_class_RdKafka_ProducerTopic(zend_class_entry *class_entry_RdKafka_Topic)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "RdKafka", "ProducerTopic", class_RdKafka_ProducerTopic_methods);
	class_entry = zend_register_internal_class_ex(&ce, class_entry_RdKafka_Topic);

	return class_entry;
}
