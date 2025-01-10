set(COMPONENT_ADD_INCLUDEDIRS .)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES esp_hap_hkdf-sha mbedtls)

set(COMPONENT_SRCS ./mu_srp.c)

register_component()
