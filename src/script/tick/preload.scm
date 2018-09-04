(define-syntax-rule (delay-run! time body ...) (delay-run time (lambda () body ...)))
(define-syntax-rule (interval-run! time body ...) (interval-run time (lambda () body ...)))