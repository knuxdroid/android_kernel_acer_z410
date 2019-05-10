define check_if_number
$(if $(1),$(if $(subst 0,,$(subst 1,,$(subst 2,,$(subst 3,,$(subst 4,,$(subst 5,,$(subst 6,,$(subst 7,,$(subst 8,,$(subst 9,,$(1))))))))))),,1))
endef
-include mediatek/config/$(PROJECT)/SimcomProjectConfig.mk
SIMCOM_FILE_JAVAOPTION_PM := mediatek/build/tools/simcomjavaoption.pm
ifneq ($(wildcard $(SIMCOM_FILE_JAVAOPTION_PM)),)
SIMCOM_LIST_JAVAOPTION_PM := $(shell cat $(SIMCOM_FILE_JAVAOPTION_PM))
else
$(error $(SIMCOM_FILE_JAVAOPTION_PM) is not exist!)
endif
SIMCOM_LIST_JAVAOPTION_DFO :=
SIMCOM_LIST_FEATUREOPTION_BOOLEAN :=
SIMCOM_LIST_FEATUREOPTION_INT :=
SIMCOM_LIST_FEATUREOPTION_BOOLEAN_DFO :=
SIMCOM_LIST_FEATUREOPTION_INT_DFO :=
SIMCOM_LIST_FEATUREOPTION_STRING :=

#only eng load will enable dfo
ifneq ($(TARGET_BUILD_VARIANT), user)
  ifneq ($(TARGET_BUILD_VARIANT), userdebug) 
    $(foreach o,$(DFO_NVRAM_SET),\
      	$(if $(filter yes,$($(o))),\
		$(eval k := $(o)_VALUE)\
		$(eval v := $($(k)))\
		$(if $(filter $(v),$(SIMCOM_LIST_JAVAOPTION_DFO)),, \
			$(eval SIMCOM_LIST_JAVAOPTION_DFO += $(v))\
		) \
	,\
		$(info ignore $(o) = $($(o)))\
	)\
    )
  endif
endif
     #echo $(SIMCOM_LIST_JAVAOPTION_PM)
$(foreach o,$(SIMCOM_LIST_JAVAOPTION_PM),\
	$(eval v := $($(o)))\
	$(if $(filter GEMINI,$(o)),\
		$(eval k := MTK_GEMINI_SUPPORT)\
	,\
		$(eval k := $(o))\
	)\
	$(if $(filter $(k),$(subst =, ,$(SIMCOM_LIST_FEATUREOPTION_BOOLEAN_DFO) $(SIMCOM_SIMCOM_LIST_FEATUREOPTION_INT_DFO) $(SIMCOM_LIST_FEATUREOPTION_BOOLEAN) $(LIST_FEATUREOPTION_INT) $(SIMCOM_SIMCOM_LIST_FEATUREOPTION_STRING))),, \
	$(if $(v),\
		$(if $(filter yes,$(v)),\
			$(if $(filter $(SIMCOM_LIST_JAVAOPTION_DFO),$(k)),\
				$(eval SIMCOM_LIST_FEATUREOPTION_BOOLEAN_DFO += $(k)=DynFeatureOption.getBoolean("$(k)"))\
			,\
				$(eval SIMCOM_LIST_FEATUREOPTION_BOOLEAN += $(k)=true)\
			)\
		,\
		$(if $(filter no,$(v)),\
			$(if $(filter $(SIMCOM_LIST_JAVAOPTION_DFO),$(k)),\
				$(eval SIMCOM_LIST_FEATUREOPTION_BOOLEAN_DFO += $(k)=DynFeatureOption.getBoolean("$(k)"))\
			,\
				$(eval SIMCOM_LIST_FEATUREOPTION_BOOLEAN += $(k)=false)\
			)\
		,\
		$(if $(call check_if_number,$(v)),\
			$(if $(filter $(SIMCOM_LIST_JAVAOPTION_DFO),$(k)),\
				$(eval SIMCOM_LIST_FEATUREOPTION_INT_DFO += $(k)=DynFeatureOption.getInt("$(k)"))\
			,\
				$(eval SIMCOM_LIST_FEATUREOPTION_INT += $(k)=$(v))\
			)\
		,\
				$(eval SIMCOM_LIST_FEATUREOPTION_STRING += $(k)="$(v)")\
		)) \
		)\
	,)\
	) \
)
k :=
v :=

SIMCOM_FILE_FEATUREOPTION_JAVA := $(SIMCOMJAVAOPTFILE)
SIMCOM_LIST_FEATUREOPTION_JAVA := NONE
ifneq ($(wildcard $(SIMCOM_FILE_FEATUREOPTION_JAVA)),)
  SIMCOM_LIST_FEATUREOPTION_JAVA := $(filter-out public static final boolean int package com.mediatek.common.featureoption; class SimcomFeatureOption { } ;,$(shell cat $(SIMCOM_FILE_FEATUREOPTION_JAVA)))
endif
k := $(strip $(filter-out $(SIMCOM_LIST_FEATUREOPTION_JAVA),$(SIMCOM_LIST_FEATUREOPTION_BOOLEAN) $(SIMCOM_LIST_FEATUREOPTION_INT) $(SIMCOM_LIST_FEATUREOPTION_BOOLEAN_DFO) $(SIMCOM_LIST_FEATUREOPTION_INT_DFO)))
v := $(strip $(filter-out $(SIMCOM_LIST_FEATUREOPTION_BOOLEAN) $(SIMCOM_LIST_FEATUREOPTION_INT) $(SIMCOM_LIST_FEATUREOPTION_BOOLEAN_DFO) $(SIMCOM_LIST_FEATUREOPTION_INT_DFO),$(SIMCOM_LIST_FEATUREOPTION_JAVA)))
ifneq ($(strip $(k))$(strip $(v)),)
$(info k = $(k))
$(info v = $(v))
.PHONY: $(SIMCOM_FILE_FEATUREOPTION_JAVA)
endif

$(SIMCOM_FILE_FEATUREOPTION_JAVA):
	@echo Update $@
	@rm -f $@
	@mkdir -p $(dir $@)
	@echo 'package com.mediatek.common.featureoption;' >>$@
	@echo 'public final class SimcomFeatureOption' >>$@
	@echo '{' >>$@
	@$(foreach o,$(SIMCOM_LIST_FEATUREOPTION_BOOLEAN),echo -e '\tpublic static final boolean $(o) ;' >>$@;)
	@$(foreach o,$(SIMCOM_LIST_FEATUREOPTION_INT),echo -e '\tpublic static final int $(o) ;' >>$@;)
	@$(foreach o,$(SIMCOM_LIST_FEATUREOPTION_BOOLEAN_DFO),echo -e '\tpublic static final boolean $(o) ;' >>$@;)
	@$(foreach o,$(SIMCOM_LIST_FEATUREOPTION_INT_DFO),echo -e '\tpublic static final int $(o) ;' >>$@;)
	@$(foreach o,$(SIMCOM_LIST_FEATUREOPTION_STRING),echo -e '\tpublic static final String $(o) ;' >>$@;)
	@echo '}' >>$@
