MAN3 = \
        mc_close.3 \
        mc_connect.3 \
        mc_flush.3 \
        mc_free.3 \
        mc_get_byte_from_bits.3 \
        mc_get_byte_timeout.3 \
        mc_get_float.3 \
        mc_get_header_length.3 \
        mc_get_response_timeout.3 \
        mc_get_socket.3 \
        mc_mapping_free.3 \
        mc_mapping_new.3 \
        mc_new_rtu.3 \
        mc_new_tcp_pi.3 \
        mc_new_tcp.3 \
        mc_read_bits.3 \
        mc_read_input_bits.3 \
        mc_read_input_registers.3 \
        mc_read_registers.3 \
        mc_receive_confirmation.3 \
        mc_receive_from.3 \
        mc_receive.3 \
        mc_reply_exception.3 \
        mc_reply.3 \
        mc_report_slave_id.3 \
        mc_rtu_get_serial_mode.3 \
        mc_rtu_set_serial_mode.3 \
        mc_send_raw_request.3 \
        mc_set_bits_from_bytes.3 \
        mc_set_bits_from_byte.3 \
        mc_set_byte_timeout.3 \
        mc_set_debug.3 \
        mc_set_error_recovery.3 \
        mc_set_float.3 \
        mc_set_response_timeout.3 \
        mc_set_slave.3 \
        mc_set_socket.3 \
        mc_strerror.3 \
        mc_tcp_listen.3 \
        mc_write_and_read_registers.3 \
        mc_write_bits.3 \
        mc_write_bit.3 \
        mc_write_registers.3 \
        mc_write_register.3
MAN7 = libmc.7

MAN_DOC = $(MAN3) $(MAN7)

MAN_TXT = $(MAN3:%.3=%.txt)
MAN_TXT += $(MAN7:%.7=%.txt)
MAN_HTML = $(MAN_TXT:%.txt=%.html)

if INSTALL_MAN
dist_man_MANS = $(MAN_DOC)
doc: $(MAN_DOC)
endif

EXTRA_DIST = asciidoc.conf $(MAN_TXT)
if BUILD_DOC
EXTRA_DIST += $(MAN_HTML)
html: $(MAN_HTML)
endif

MAINTAINERCLEANFILES = $(MAN_DOC) $(MAN_HTML)

dist-hook: $(MAN_DOC) $(MAN_HTML)

if BUILD_DOC
SUFFIXES=.html .txt .xml .1 .3 .7

.txt.html:
	asciidoc -d manpage -b xhtml11 -f asciidoc.conf \
		-alibmc_version=@LIBMC_VERSION@ $<
.txt.xml:
	asciidoc -d manpage -b docbook -f asciidoc.conf \
		-alibmc_version=@LIBMC_VERSION@ $<
.xml.1:
	xmlto man $<
.xml.3:
	xmlto man $<
.xml.7:
	xmlto man $<

clean:
	rm -f *.1 *.3 *.7
	rm -f *.html

endif
