(define-module (custom block-items)
               #:use-module (minecraft)
               #:use-module (minecraft base)
               #:use-module (minecraft policy)
               #:use-module (minecraft chat)
               
               #:export (init-block-items))

(define (init-block-items blacklist-item)
        (add-hook! policy-player-use-on
           (lambda (item pos vec)
                   (if (member (item-instance-id item) blacklist-item)
                       (begin (policy-result #f)
                              (send-message (policy-self) (format #f "You cannot place ~a here" (item-instance-name item))))))))


