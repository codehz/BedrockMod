(use-modules (minecraft))
(use-modules (system foreign))

(define ptr (quote *))

(define (char->type ch)
        (case ch
              ((#\y) uint8)
              ((#\b) int8)
              ((#\n) int16)
              ((#\q) uint16)
              ((#\i) int32)
              ((#\u) uint32)
              ((#\x) int64)
              ((#\t) uint64)
              ((#\d) double)
              ((#\s) ptr)))

(define (string->typelist str) (map char->type (string->list str)))

(define libsystemd (dynamic-link "libsystemd"))

(define sd-bus-method-return (dynamic-func "sd_bus_reply_method_return" libsystemd))

(define (dbus-reply msg type . args)
        (let ((func     (pointer->procedure int
                                            sd-bus-method-return
                                            (append (list ptr ptr) (string->typelist type)))))
             (apply func msg (string->pointer type) args)))

(define (register-dbus-interface path object fn)
        (let ((vt    (make-dbus-vtable))
              (fpath (string-append "/one/codehz/bedrockserver" path)))
             (fn vt)
             (add-obj-vtable fpath object vt)))

(define (define-dbus-method vt flags name sig res handler)
        (let ((func (procedure->pointer void
                                        handler
                                        (list ptr ptr ptr))))
             (define-dbus-method-internal vt flags name sig res func)))

(export dbus-reply register-dbus-interface define-dbus-method)

(define ptr (quote *))