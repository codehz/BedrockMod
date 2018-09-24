(define-module (tests database)
               #:use-module (minecraft)

               #:use-module (sqlite3))

(define memdb (sqlite-open ":memory:"))

(sqlite-exec memdb "CREATE TABLE TEST(key text primary key, value text);")
(define (insert-kv k v)
        (let [(stmt (sqlite-prepare memdb "INSERT INTO TEST VALUES(?, ?);" #:cache? #t))]
              (sqlite-bind-arguments stmt k v)
              (sqlite-step stmt)))

(insert-kv "1" "2")
(insert-kv "3" "24")
(insert-kv "4" "2t")
(insert-kv "5" "2we")
(insert-kv "6" "2tr")

(let [(stmt (sqlite-prepare memdb "SELECT * FROM TEST;"))]
      (log-debug "SQLITE" "~a" (sqlite-map identity stmt)))

(sqlite-close memdb)