find_program(LRELEASE_CMD_PATH NAMES lrelease-qt5 lrelease)
set(LRELEASE_CMD ${LRELEASE_CMD_PATH})
if (LRELEASE_CMD)
    message(STATUS "Found lrelease at ${LRELEASE_CMD}")
    message(STATUS "Translations are enabled")

    set (LCPREFIX ".")
    set (PATHSEP "/")

    set (PARTS
        cutelystcmd
        cutelystwsgi
        cutelystcore
        plugin_memcached
        plugin_csrfprotection
        plugin_view_grantlee
        plugin_utils_validator)

    foreach(PART ${PARTS})
        string(CONCAT GLOBEXPRESSION ${PART} ${LCPREFIX} "*.ts")
        file(GLOB TRANSLATIONS "${CMAKE_CURRENT_SOURCE_DIR}/${GLOBEXPRESSION}")
        foreach(TRANS ${TRANSLATIONS})
            string(COMPARE NOTEQUAL ${TRANS} ${CMAKE_CURRENT_SOURCE_DIR} NOTTHEDIR)
            string(CONCAT SOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR} ${PATHSEP} ${PART} ".ts")
            string(COMPARE NOTEQUAL ${TRANS} ${SOURCE_FILE} NOTTHESOURCE)
            if (NOTTHEDIR AND NOTTHESOURCE)
                string(LENGTH ${CMAKE_CURRENT_SOURCE_DIR} CURRENT_SOURCE_DIR_LENGTH)
                string(LENGTH ${PART} PART_LENGTH)
                string(LENGTH ${TRANS} TRANS_LENGTH)
                string(LENGTH ${LCPREFIX} LCPREFIX_LENGTH)
                math(EXPR SUBSTART ${CURRENT_SOURCE_DIR_LENGTH}+${PART_LENGTH}+${LCPREFIX_LENGTH}+1)
                math(EXPR SUBLENGTH ${TRANS_LENGTH}-${CURRENT_SOURCE_DIR_LENGTH}-${PART_LENGTH}-${LCPREFIX_LENGTH}-4)
                string(SUBSTRING ${TRANS} ${SUBSTART} ${SUBLENGTH} LC)
                execute_process(COMMAND ${LRELEASE_CMD} -silent ${TRANS} -qm ${CMAKE_CURRENT_BINARY_DIR}${PATHSEP}${PART}${LCPREFIX}${LC}.qm RESULT_VARIABLE LRELEASE_RESULT ERROR_VARIABLE LRELEASE_ERROR)
                if (${LRELEASE_RESULT} EQUAL 0)
                    list(APPEND QM_FILES ${CMAKE_CURRENT_BINARY_DIR}${PATHSEP}${PART}${LCPREFIX}${LC}.qm)
                else (${LRELEASE_RESULT} EQUAL 0)
                    message(WARNING ${LRELEASE_ERROR})
                endif (${LRELEASE_RESULT} EQUAL 0)
            endif (NOTTHEDIR AND NOTTHESOURCE)
        endforeach(TRANS)
    endforeach(PART)

    install (FILES ${QM_FILES} DESTINATION ${I18NDIR})

else (LRELEASE_CMD)
    message(WARNING "lrelease executable cannot be found")
    message(WARNING "Translations are disabled")
endif (LRELEASE_CMD)
