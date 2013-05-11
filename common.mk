$(LOCAL_PATH)/doc.c: local_README:=$(LOCAL_PATH)/README.md
$(LOCAL_PATH)/doc.c: $(LOCAL_PATH)/README.md $(LOCAL_PATH)/common.mk
	echo "const char sampler_doc[] =" > ${@}
	cat $(local_README) | sed -e 's/"/\\\"/g' | sed -e 's/^/"/' | sed -e 's/$$/\\n"/' >> ${@}
	echo ";" >> ${@}

common-clean:
	rm -f $(LOCAL_PATH)/doc.c
