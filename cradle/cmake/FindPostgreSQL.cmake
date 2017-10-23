# find PostgreSQL

MESSAGE(FindPostgreSQL)

SET(INCLUDE_PREFIX_DIR "C:/libs-vc14-x64/postgresql/include/")
SET(POSTGRESQL_INCLUDE_DIR
	"${INCLUDE_PREFIX_DIR}"
	"${INCLUDE_PREFIX_DIR}/informix"
	"${INCLUDE_PREFIX_DIR}/internal"
	"${INCLUDE_PREFIX_DIR}/libpq"
	"${INCLUDE_PREFIX_DIR}/server"
	)

SET(LIBRARY_PREFIX_DIR "C:/libs-vc14-x64/postgresql/lib/")
SET(POSTGRESQL_LIBRARIES
	"${LIBRARY_PREFIX_DIR}libecpg.lib"
	"${LIBRARY_PREFIX_DIR}libecpg_compat.lib"
	"${LIBRARY_PREFIX_DIR}libpgcommon.lib"
	"${LIBRARY_PREFIX_DIR}libpgport.lib"
	"${LIBRARY_PREFIX_DIR}libpgtypes.lib"
	"${LIBRARY_PREFIX_DIR}libpq.lib"
	"${LIBRARY_PREFIX_DIR}postgres.lib"
	) 