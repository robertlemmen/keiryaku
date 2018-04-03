(length '('() #f 3))
(length '(1))
(length '())

(define mv1 (make-vector 7 3))
(vector? mv1)
(vector-length mv1);
(vector-ref mv1 0)
(vector-ref mv1 6)
(vector-set! mv1 2 #f)
(vector-ref mv1 2)
(vector-ref mv1 3)

(define mv2 (make-vector 5))
(vector? mv2)
(vector-length mv2);
(vector-ref mv2 2)

(define mv3 (vector 1 2 3 #t))
(vector? mv3)
(vector-length mv3)
(vector-ref mv3 0)
(vector-ref mv3 3)

(define mv4 #(1 2 3))
(vector? mv4)
(vector-length mv4);
(vector-ref mv4 2)
#(1 2 3 4)
===
3
1
0
#t
7
3
3
#f
3
#t
5
0
#t
4
1
#t
#t
3
3
#(1 2 3 4)
