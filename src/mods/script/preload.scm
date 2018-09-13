(define-public (log-trace  tag msg . args) (log-raw log-level-trace  tag (apply format #f msg args)))
(define-public (log-debug  tag msg . args) (log-raw log-level-debug  tag (apply format #f msg args)))
(define-public (log-info   tag msg . args) (log-raw log-level-info   tag (apply format #f msg args)))
(define-public (log-notice tag msg . args) (log-raw log-level-notice tag (apply format #f msg args)))
(define-public (log-warn   tag msg . args) (log-raw log-level-warn   tag (apply format #f msg args)))
(define-public (log-error  tag msg . args) (log-raw log-level-error  tag (apply format #f msg args)))
(define-public (log-fatal  tag msg . args) (log-raw log-level-fatal  tag (apply format #f msg args)))

(set! %load-path '("scm/scripts"))