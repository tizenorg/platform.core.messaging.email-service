# for i18n

#SET(POFILES en_US.po en_GB.po ja.po ko.po zh_CN.po)
#SET(POFILES de.po el.po en_US.po es.po fr.po it.po ja.po ko.po nl.po pt.po ru.po tk.po zh_CN.po zh_HK.po zh_TW.po)
SET(POFILES en.po nl_NL.po de_DE.po zh_HK.po zh_CN.po ru_RU.po ko_KR.po zh_TW.po ja_JP.po es_ES.po el_GR.po it_IT.po tr_TR.po pt_PT.po fr_FR.po)

SET(MSGFMT "/usr/bin/msgfmt")

FOREACH(pofile ${POFILES})
	SET(pofile ${CMAKE_CURRENT_SOURCE_DIR}/${pofile})
	MESSAGE("PO: ${pofile}")
	GET_FILENAME_COMPONENT(absPofile ${pofile} ABSOLUTE)
	GET_FILENAME_COMPONENT(lang ${absPofile} NAME_WE)
	SET(moFile ${CMAKE_CURRENT_BINARY_DIR}/${lang}.mo)
	ADD_CUSTOM_COMMAND(
			OUTPUT ${moFile}
			COMMAND ${MSGFMT} -o ${moFile} ${absPofile}
			DEPENDS ${absPofile}
	)

	INSTALL(FILES ${moFile}
			DESTINATION /opt/apps/email-service/res/locale/${lang}/LC_MESSAGES RENAME email.mo)

	SET(moFiles ${moFiles} ${moFile})
ENDFOREACH(pofile)

MESSAGE(".mo files: ${moFiles}")
ADD_CUSTOM_TARGET(po ALL DEPENDS ${moFiles})
