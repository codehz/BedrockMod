(define-module (custom block-items)
               #:use-module (minecraft)
               #:use-module (minecraft base)
               #:use-module (minecraft policy)
               #:use-module (minecraft chat))

(define blacklist-item '(7 -161))

(add-hook! policy-player-use-on
           (lambda (item pos vec)
                   (if (member (item-instance-id item) blacklist-item)
                       (begin (policy-result #f)
                              (send-message (policy-self) (format #f "You cannot place ~a here" (item-instance-name item)))))))
