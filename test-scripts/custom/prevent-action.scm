(define-module (custom prevent-action)
               #:use-module (minecraft)
               #:use-module (minecraft base)
               #:use-module (minecraft policy)
               #:use-module (minecraft world)
               #:use-module (minecraft chat)
               
               #:export (prevent-item-use prevent-block-destroy))

(define (prevent-item-use blacklist-item)
        (add-hook! policy-player-use-on
           (lambda (item pos vec)
                   (if (member (item-instance-id item) blacklist-item)
                       (begin (policy-result #f)
                              (send-message (policy-self) (format #f "You cannot place ~a here" (item-instance-name item))))))))


(define (prevent-block-destroy unbreakable-blocks)
        (add-hook! policy-player-destroy
           (lambda (pos)
                   (if (member (block-name (get-block/player pos (policy-self))) unbreakable-blocks)
                       (begin (policy-result #f)
                              (send-message (policy-self) (format #f "You cannot break ~a here" (block-name (get-block/player pos (policy-self))))))))))


