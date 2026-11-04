#!/bin/sh

docker network create kafka_network
docker pull wurstmeister/zookeeper:latest
docker run -d --network kafka_network --name zookeeper wurstmeister/zookeeper:latest
docker pull wurstmeister/kafka:latest
docker run -d -p 9092:9092 --network kafka_network \
    -e "KAFKA_AUTO_CREATE_TOPICS_ENABLE=true" \
    -e "KAFKA_CREATE_TOPICS=test-topic:1:1:compact" \
    -e "KAFKA_BROKER_ID=1" \
    -e "KAFKA_ADVERTISED_HOST_NAME=kafka" \
    -e "KAFKA_ZOOKEEPER_CONNECT=zookeeper:2181/kafka" \
    -e "KAFKA_ADVERTISED_PORT=9092" \
    --name kafka wurstmeister/kafka:latest

if [ ${SKIP_OAUTH:-0} -ne 1 ]; then
  docker run -d -p 29092:29092 --network kafka_network \
      -e "KAFKA_AUTO_CREATE_TOPICS_ENABLE=true" \
      -e "KAFKA_CREATE_TOPICS=test-topic:1:1:compact" \
      -e "KAFKA_ADVERTISED_HOST_NAME=kafka_oauth2" \
      -e "KAFKA_ZOOKEEPER_CONNECT=zookeeper:2181/kafka_oauth2" \
      -e "KAFKA_ADVERTISED_PORT=29092" \
      -e "KAFKA_BROKER_ID=2" \
      -e "KAFKA_LISTENERS=SASLPLAINTEXT://kafka_oauth2:29092" \
      -e "KAFKA_ADVERTISED_LISTENERS=SASLPLAINTEXT://kafka_oauth2:29092" \
      -e "KAFKA_LISTENER_NAME_SASLPLAINTEXT_OAUTHBEARER_SASL_JAAS_CONFIG=org.apache.kafka.common.security.oauthbearer.OAuthBearerLoginModule required unsecuredValidatorRequiredScope=\"required-scope\" unsecuredLoginStringClaim_scope=\"required-scope\" unsecuredLoginStringClaim_sub=\"admin\";" \
      -e "KAFKA_INTER_BROKER_LISTENER_NAME=SASLPLAINTEXT" \
      -e "KAFKA_SASL_ENABLED_MECHANISMS=OAUTHBEARER" \
      -e "KAFKA_LISTENER_SECURITY_PROTOCOL_MAP=SASLPLAINTEXT:SASL_PLAINTEXT" \
      -e "KAFKA_SASL_MECHANISM_INTER_BROKER_PROTOCOL=OAUTHBEARER" \
      --name kafka_oauth2 wurstmeister/kafka:latest
fi

printf "\n127.0.0.1  kafka\n127.0.0.1  kafka_oauth2\n"|sudo tee /etc/hosts >/dev/null

echo "Waiting for Kafka services to be ready"

kafka_ready=0
kafka_oauth2_ready=0

for i in $(seq 1 20); do
    if [ $kafka_ready -eq 0 ]; then
        if kafkacat -b kafka:9092 -L -m 30 -d broker; then
            kafka_ready=1
            echo "Kafka is ready"
        fi
    fi
    if [ $kafka_oauth2_ready -eq 0 ] && [ ${SKIP_OAUTH:-0} -ne 1 ]; then
        if kafkacat -b kafka_oauth2:29092 \
            -X security.protocol=SASL_PLAINTEXT \
            -X sasl.mechanisms=OAUTHBEARER \
            -X enable.sasl.oauthbearer.unsecure.jwt="true" \
            -X sasl.oauthbearer.config="principal=admin scope=required-scope" -L
        then
            kafka_oauth2_ready=1
            echo "Kafka OAuth2 is ready"
        fi
    fi

    if [ $kafka_ready -eq 1 ] && ( [ $kafka_oauth2_ready -eq 1 ] || [ ${SKIP_OAUTH:-0} -eq 1 ] ); then
      exit 0
    fi
done

echo "Timedout waiting for Kafka services to be ready"
exit 1
