LOCAL_IMAGE ?= openshift-spark-inc
SPARK_IMAGE=radanalyticsio/openshift-spark-inc

LOCAL_IMAGE_PY36 ?= openshift-spark-py36-inc
SPARK_IMAGE_PY36=radanalyticsio/openshift-spark-py36-inc

DOCKERFILE_CONTEXT=openshift-spark-build-inc
DOCKERFILE_CONTEXT_PY36=openshift-spark-build-py36-inc

SPARK_TEST_IMAGE ?= spark-testimage-inc
SPARK_TEST_IMAGE_PY36 ?= spark-testimage-py36-inc

export SPARK_TEST_IMAGE

.PHONY: build clean push create destroy test-e2e test-e2e-py test-e2e-py-completed test-e2e-py36 test-e2e-py36-completed build-py build-py36

build: build-py build-py36

build-py: $(DOCKERFILE_CONTEXT)
	docker build -t $(LOCAL_IMAGE) $(DOCKERFILE_CONTEXT)

build-py36: $(DOCKERFILE_CONTEXT_PY36)
	docker build -t $(LOCAL_IMAGE_PY36) $(DOCKERFILE_CONTEXT_PY36)

push: build
	docker tag $(LOCAL_IMAGE) $(SPARK_IMAGE)
	docker push $(SPARK_IMAGE)
	docker tag $(LOCAL_IMAGE_PY36) $(SPARK_IMAGE_PY36)
	docker push $(SPARK_IMAGE_PY36)

clean: clean-context
	-docker rmi $(LOCAL_IMAGE)
	-docker rmi $(LOCAL_IMAGE_PY36)

clean-target:
	-rm -rf target
	-rm -rf target-py36

clean-context:
	-rm -rf $(DOCKERFILE_CONTEXT)/*
	-rm -rf $(DOCKERFILE_CONTEXT_PY36)/*

context: $(DOCKERFILE_CONTEXT) $(DOCKERFILE_CONTEXT_PY36)

$(DOCKERFILE_CONTEXT): $(DOCKERFILE_CONTEXT)/Dockerfile $(DOCKERFILE_CONTEXT)/modules

$(DOCKERFILE_CONTEXT_PY36): $(DOCKERFILE_CONTEXT_PY36)/Dockerfile $(DOCKERFILE_CONTEXT_PY36)/modules

$(DOCKERFILE_CONTEXT)/Dockerfile $(DOCKERFILE_CONTEXT)/modules:
	-mkdir -p $(DOCKERFILE_CONTEXT)
	cekit generate --descriptor image-inc.yaml --overrides overrides/default.yaml
	cp -R target/image/* $(DOCKERFILE_CONTEXT)
	-rm $(DOCKERFILE_CONTEXT)/spark*.tgz

$(DOCKERFILE_CONTEXT_PY36)/Dockerfile $(DOCKERFILE_CONTEXT_PY36)/modules:
	-mkdir -p $(DOCKERFILE_CONTEXT_PY36)
	cekit generate --descriptor image-inc.yaml --overrides overrides/python36-inc.yaml --target target-py36
	cp -R target-py36/image/* $(DOCKERFILE_CONTEXT_PY36)
	-rm $(DOCKERFILE_CONTEXT_PY36)/spark*.tgz

zero-tarballs:
	find ./$(DOCKERFILE_CONTEXT) -name "*.tgz" -type f -exec truncate -s 0 {} \;
	find ./$(DOCKERFILE_CONTEXT) -name "*.tar.gz" -type f -exec truncate -s 0 {} \;
	find ./$(DOCKERFILE_CONTEXT_PY36) -name "*.tgz" -type f -exec truncate -s 0 {} \;
	find ./$(DOCKERFILE_CONTEXT_PY36) -name "*.tar.gz" -type f -exec truncate -s 0 {} \;

test-e2e:
	test/sparkinputs.sh
	LOCAL_IMAGE=$(SPARK_TEST_IMAGE) LOCAL_IMAGE_PY36=$(SPARK_TEST_IMAGE_PY36) make -f Makefile.inc build
	SPARK_TEST_IMAGE=$(SPARK_TEST_IMAGE) test/run.sh incomplete/
	SPARK_TEST_IMAGE=$(SPARK_TEST_IMAGE_PY36) test/run.sh incomplete/

test-e2e-py:
	test/sparkinputs.sh
	LOCAL_IMAGE=$(SPARK_TEST_IMAGE) make -f Makefile.inc build-py
	SPARK_TEST_IMAGE=$(SPARK_TEST_IMAGE) test/run.sh incomplete/

test-e2e-py-completed:
	test/sparkinputs.sh
	LOCAL_IMAGE=$(SPARK_TEST_IMAGE) make -f Makefile.inc build-py
	test/localcomplete.sh $(SPARK_TEST_IMAGE) spark-complete
	SPARK_TEST_IMAGE=spark-complete test/run.sh completed/

test-e2e-py36:
	test/sparkinputs.sh
	LOCAL_IMAGE_PY36=$(SPARK_TEST_IMAGE_PY36) make -f Makefile.inc build-py36
	SPARK_TEST_IMAGE=$(SPARK_TEST_IMAGE_PY36) test/run.sh incomplete/

test-e2e-py36-completed:
	test/sparkinputs.sh
	LOCAL_IMAGE_PY36=$(SPARK_TEST_IMAGE_PY36) make -f Makefile.inc build-py36
	test/localcomplete.sh $(SPARK_TEST_IMAGE_PY36) spark-complete-py36
	SPARK_TEST_IMAGE=spark-complete-py36 test/run.sh completed/
