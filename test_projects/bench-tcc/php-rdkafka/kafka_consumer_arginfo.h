/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 47e9238c79f5508833423d31a2e09041754dbffb */

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RdKafka_KafkaConsumer___construct, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, conf, RdKafka\\Conf, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_RdKafka_KafkaConsumer_assign, 0, 0, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, topic_partitions, IS_ARRAY, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_RdKafka_KafkaConsumer_getAssignment, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_RdKafka_KafkaConsumer_commit, 0, 0, IS_VOID, 0)
	ZEND_ARG_OBJ_TYPE_MASK(0, message_or_offsets, RdKafka\\Message, MAY_BE_ARRAY|MAY_BE_NULL, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_RdKafka_KafkaConsumer_close, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RdKafka_KafkaConsumer_commitAsync arginfo_class_RdKafka_KafkaConsumer_commit

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_OBJ_INFO_EX(arginfo_class_RdKafka_KafkaConsumer_consume, 0, 1, RdKafka\\Message, 0)
	ZEND_ARG_TYPE_INFO(0, timeout_ms, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_RdKafka_KafkaConsumer_subscribe, 0, 1, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, topics, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RdKafka_KafkaConsumer_getSubscription arginfo_class_RdKafka_KafkaConsumer_getAssignment

#define arginfo_class_RdKafka_KafkaConsumer_unsubscribe arginfo_class_RdKafka_KafkaConsumer_close

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_OBJ_INFO_EX(arginfo_class_RdKafka_KafkaConsumer_getMetadata, 0, 3, RdKafka\\Metadata, 0)
	ZEND_ARG_TYPE_INFO(0, all_topics, _IS_BOOL, 0)
	ZEND_ARG_OBJ_INFO(0, only_topic, RdKafka\\Topic, 1)
	ZEND_ARG_TYPE_INFO(0, timeout_ms, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_OBJ_INFO_EX(arginfo_class_RdKafka_KafkaConsumer_newTopic, 0, 1, RdKafka\\KafkaConsumerTopic, 0)
	ZEND_ARG_TYPE_INFO(0, topic_name, IS_STRING, 0)
	ZEND_ARG_OBJ_INFO_WITH_DEFAULT_VALUE(0, topic_conf, RdKafka\\TopicConf, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_RdKafka_KafkaConsumer_getCommittedOffsets, 0, 2, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, topic_partitions, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, timeout_ms, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_RdKafka_KafkaConsumer_getOffsetPositions, 0, 1, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, topic_partitions, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_RdKafka_KafkaConsumer_queryWatermarkOffsets, 0, 5, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, topic, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, partition, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(1, low, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(1, high, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, timeout_ms, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RdKafka_KafkaConsumer_offsetsForTimes arginfo_class_RdKafka_KafkaConsumer_getCommittedOffsets

#define arginfo_class_RdKafka_KafkaConsumer_pausePartitions arginfo_class_RdKafka_KafkaConsumer_getOffsetPositions

#define arginfo_class_RdKafka_KafkaConsumer_resumePartitions arginfo_class_RdKafka_KafkaConsumer_getOffsetPositions


ZEND_METHOD(RdKafka_KafkaConsumer, __construct);
ZEND_METHOD(RdKafka_KafkaConsumer, assign);
ZEND_METHOD(RdKafka_KafkaConsumer, getAssignment);
ZEND_METHOD(RdKafka_KafkaConsumer, commit);
ZEND_METHOD(RdKafka_KafkaConsumer, close);
ZEND_METHOD(RdKafka_KafkaConsumer, commitAsync);
ZEND_METHOD(RdKafka_KafkaConsumer, consume);
ZEND_METHOD(RdKafka_KafkaConsumer, subscribe);
ZEND_METHOD(RdKafka_KafkaConsumer, getSubscription);
ZEND_METHOD(RdKafka_KafkaConsumer, unsubscribe);
ZEND_METHOD(RdKafka_KafkaConsumer, getMetadata);
ZEND_METHOD(RdKafka_KafkaConsumer, newTopic);
ZEND_METHOD(RdKafka_KafkaConsumer, getCommittedOffsets);
ZEND_METHOD(RdKafka_KafkaConsumer, getOffsetPositions);
ZEND_METHOD(RdKafka_KafkaConsumer, queryWatermarkOffsets);
ZEND_METHOD(RdKafka_KafkaConsumer, offsetsForTimes);
ZEND_METHOD(RdKafka_KafkaConsumer, pausePartitions);
ZEND_METHOD(RdKafka_KafkaConsumer, resumePartitions);


static const zend_function_entry class_RdKafka_KafkaConsumer_methods[] = {
	ZEND_ME(RdKafka_KafkaConsumer, __construct, arginfo_class_RdKafka_KafkaConsumer___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_KafkaConsumer, assign, arginfo_class_RdKafka_KafkaConsumer_assign, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_KafkaConsumer, getAssignment, arginfo_class_RdKafka_KafkaConsumer_getAssignment, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_KafkaConsumer, commit, arginfo_class_RdKafka_KafkaConsumer_commit, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_KafkaConsumer, close, arginfo_class_RdKafka_KafkaConsumer_close, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_KafkaConsumer, commitAsync, arginfo_class_RdKafka_KafkaConsumer_commitAsync, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_KafkaConsumer, consume, arginfo_class_RdKafka_KafkaConsumer_consume, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_KafkaConsumer, subscribe, arginfo_class_RdKafka_KafkaConsumer_subscribe, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_KafkaConsumer, getSubscription, arginfo_class_RdKafka_KafkaConsumer_getSubscription, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_KafkaConsumer, unsubscribe, arginfo_class_RdKafka_KafkaConsumer_unsubscribe, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_KafkaConsumer, getMetadata, arginfo_class_RdKafka_KafkaConsumer_getMetadata, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_KafkaConsumer, newTopic, arginfo_class_RdKafka_KafkaConsumer_newTopic, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_KafkaConsumer, getCommittedOffsets, arginfo_class_RdKafka_KafkaConsumer_getCommittedOffsets, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_KafkaConsumer, getOffsetPositions, arginfo_class_RdKafka_KafkaConsumer_getOffsetPositions, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_KafkaConsumer, queryWatermarkOffsets, arginfo_class_RdKafka_KafkaConsumer_queryWatermarkOffsets, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_KafkaConsumer, offsetsForTimes, arginfo_class_RdKafka_KafkaConsumer_offsetsForTimes, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_KafkaConsumer, pausePartitions, arginfo_class_RdKafka_KafkaConsumer_pausePartitions, ZEND_ACC_PUBLIC)
	ZEND_ME(RdKafka_KafkaConsumer, resumePartitions, arginfo_class_RdKafka_KafkaConsumer_resumePartitions, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static zend_class_entry *register_class_RdKafka_KafkaConsumer(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "RdKafka", "KafkaConsumer", class_RdKafka_KafkaConsumer_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);

	zval property_error_cb_default_value;
	ZVAL_UNDEF(&property_error_cb_default_value);
	zend_string *property_error_cb_name = zend_string_init("error_cb", sizeof("error_cb") - 1, 1);
	zend_declare_typed_property(class_entry, property_error_cb_name, &property_error_cb_default_value, ZEND_ACC_PRIVATE, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_CALLABLE|MAY_BE_NULL));
	zend_string_release(property_error_cb_name);

	zval property_rebalance_cb_default_value;
	ZVAL_UNDEF(&property_rebalance_cb_default_value);
	zend_string *property_rebalance_cb_name = zend_string_init("rebalance_cb", sizeof("rebalance_cb") - 1, 1);
	zend_declare_typed_property(class_entry, property_rebalance_cb_name, &property_rebalance_cb_default_value, ZEND_ACC_PRIVATE, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_CALLABLE|MAY_BE_NULL));
	zend_string_release(property_rebalance_cb_name);

	zval property_dr_msg_cb_default_value;
	ZVAL_UNDEF(&property_dr_msg_cb_default_value);
	zend_string *property_dr_msg_cb_name = zend_string_init("dr_msg_cb", sizeof("dr_msg_cb") - 1, 1);
	zend_declare_typed_property(class_entry, property_dr_msg_cb_name, &property_dr_msg_cb_default_value, ZEND_ACC_PRIVATE, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_CALLABLE|MAY_BE_NULL));
	zend_string_release(property_dr_msg_cb_name);

	return class_entry;
}
