
(define member?
  (lambda (a lat)
    (cond
      ((null? lat) #f)
      (else (or (eq? (car lat) a)
                (member? a (cdr lat)))))))

(define atom?
 (lambda (x)
    (and (not (pair? x)) (not (null? x)))))

'(apples peaches pears plums)

'(apple peaches apple plum)

(define set?
  (lambda (lat)
    (cond
      ((null? lat) #t)
      ((member? (car lat) (cdr lat)) #f)
      (else
        (set? (cdr lat))))))

(set? '(apples peaches pears plums))
(set? '(apple peaches apple plum))
(set? '(apple 3 pear 4 9 apple 3 4))

(define makeset
  (lambda (lat)
    (cond
      ((null? lat) '())
      ((member? (car lat) (cdr lat)) (makeset (cdr lat)))
      (else
        (cons (car lat) (makeset (cdr lat)))))))

(makeset '(apple peach pear peach plum apple lemon peach))

(define multirember
  (lambda (a lat)
    (cond
      ((null? lat) '())
      ((eq? (car lat) a)
       (multirember a (cdr lat)))
      (else
        (cons (car lat) (multirember a (cdr lat)))))))

(define makeset
  (lambda (lat)
    (cond
      ((null? lat) '())
      (else
        (cons (car lat)
              (makeset (multirember (car lat) (cdr lat))))))))

(makeset '(apple peach pear peach plum apple lemon peach))

(makeset '(apple 3 pear 4 9 apple 3 4))

(define subset?
  (lambda (set1 set2)
    (cond
      ((null? set1) #t)
      ((member? (car set1) set2)
       (subset? (cdr set1) set2))
      (else #f))))

(subset? '(5 chicken wings)
         '(5 hamburgers 2 pieces fried chicken and light duckling wings))

(subset? '(4 pounds of horseradish)
         '(four pounds of chicken and 5 ounces of horseradish))

(define subset?
  (lambda (set1 set2)
    (cond
      ((null? set1) #t)
      (else (and (member? (car set1) set2)
                 (subset? (cdr set1) set2))))))

(subset? '(5 chicken wings)
         '(5 hamburgers 2 pieces fried chicken and light duckling wings))

(subset? '(4 pounds of horseradish)
         '(four pounds of chicken and 5 ounces of horseradish))

(define eqset?
  (lambda (set1 set2)
    (and (subset? set1 set2)
         (subset? set2 set1))))

(eqset? '(a b c) '(c b a))
(eqset? '() '())
(eqset? '(a b c) '(a b))

(define intersect?
  (lambda (set1 set2)
    (cond
      ((null? set1) #f)
      ((member? (car set1) set2) #t)
      (else
        (intersect? (cdr set1) set2)))))

(intersect?
  '(stewed tomatoes and macaroni)
  '(macaroni and cheese))

(intersect?
  '(a b c)
  '(d e f))

(define intersect?
  (lambda (set1 set2)
    (cond
      ((null? set1) #f)
      (else (or (member? (car set1) set2)
                (intersect? (cdr set1) set2))))))

(intersect?
  '(stewed tomatoes and macaroni)
  '(macaroni and cheese))

(intersect?
  '(a b c)
  '(d e f))

(define intersect
  (lambda (set1 set2)
    (cond
      ((null? set1) '())
      ((member? (car set1) set2)
       (cons (car set1) (intersect (cdr set1) set2)))
      (else
        (intersect (cdr set1) set2)))))

(intersect
  '(stewed tomatoes and macaroni)
  '(macaroni and cheese))

(define union
  (lambda (set1 set2)
    (cond
      ((null? set1) set2)
      ((member? (car set1) set2)
       (union (cdr set1) set2))
      (else (cons (car set1) (union (cdr set1) set2))))))

(union
  '(stewed tomatoes and macaroni casserole)
  '(macaroni and cheese))

(define xxx
  (lambda (set1 set2)
    (cond
      ((null? set1) '())
      ((member? (car set1) set2)
       (xxx (cdr set1) set2))
      (else
        (cons (car set1) (xxx (cdr set1) set2))))))

(xxx '(a b c) '(a b d e f))

(define intersectall
  (lambda (l-set)
    (cond
      ((null? (cdr l-set)) (car l-set))
      (else
        (intersect (car l-set) (intersectall (cdr l-set)))))))

(intersectall '((a b c) (c a d e) (e f g h a b)))
(intersectall
  '((6 pears and)
    (3 peaches and 6 peppers)
    (8 pears and 6 plums)
    (and 6 prunes with some apples)))

(define a-pair?
  (lambda (x)
    (cond
      ((atom? x) #f)
      ((null? x) #f)
      ((null? (cdr x)) #f)
      ((null? (cdr (cdr x))) #t)
      (else #f))))

(a-pair? '(pear pear))
(a-pair? '(3 7))
(a-pair? '((2) (pair)))
(a-pair? '(full (house)))

(a-pair? '())
(a-pair? '(a b c))

(define first
  (lambda (p)
    (car p)))

(define second
  (lambda (p)
    (car (cdr p))))

(define build
  (lambda (s1 s2)
    (cons s1 (cons s2 '()))))

(define third
  (lambda (l)
    (car (cdr (cdr l)))))

'(apples peaches pumpkins pie)
'((apples peaches) (pumpkin pie) (apples peaches))

'((apples peaches) (pumpkin pie))
'((4 3) (4 2) (7 6) (6 2) (3 4))

(define firsts
  (lambda (l)
    (cond
      ((null? l) '())
      (else
        (cons (car (car l)) (firsts (cdr l)))))))

(define fun?
  (lambda (rel)
    (set? (firsts rel))))

(define firsts
  (lambda (l)
    (cond
      ((null? l) '())
      (else
        (cons (car (car l)) (firsts (cdr l)))))))

(fun? '((4 3) (4 2) (7 6) (6 2) (3 4)))
(fun? '((8 3) (4 2) (7 6) (6 2) (3 4)))
(fun? '((d 4) (b 0) (b 9) (e 5) (g 4)))

(define revrel
  (lambda (rel)
    (cond
      ((null? rel) '())
      (else (cons (build (second (car rel))
                         (first (car rel)))
                  (revrel (cdr rel)))))))

(revrel '((8 a) (pumpkin pie) (got sick)))

(define revpair
  (lambda (p)
    (build (second p) (first p))))

(define revrel
  (lambda (rel)
    (cond
      ((null? rel) '())
      (else (cons (revpair (car rel)) (revrel (cdr rel)))))))

(revrel '((8 a) (pumpkin pie) (got sick)))

(define seconds
  (lambda (l)
    (cond
      ((null? l) '())
      (else
        (cons (second (car l)) (seconds (cdr l)))))))

(define fullfun?
  (lambda (fun)
    (set? (seconds fun))))

(fullfun? '((8 3) (4 2) (7 6) (6 2) (3 4)))
(fullfun? '((8 3) (4 8) (7 6) (6 2) (3 4)))
(fullfun? '((grape raisin)
            (plum prune)
            (stewed prune)))

(define one-to-one?
  (lambda (fun)
    (fun? (revrel fun))))

(one-to-one? '((8 3) (4 2) (7 6) (6 2) (3 4)))
(one-to-one? '((8 3) (4 8) (7 6) (6 2) (3 4)))
(one-to-one? '((grape raisin)
               (plum prune)
               (stewed prune)))

(one-to-one? '((chocolate chip) (doughy cookie)))

===
(apples peaches pears plums)
(apple peaches apple plum)
#t
#f
#f
(pear plum apple lemon peach)
(apple peach pear plum lemon)
(apple 3 pear 4 9)
#t
#f
#t
#f
#t
#t
#f
#t
#f
#t
#f
(and macaroni)
(stewed tomatoes casserole macaroni and cheese)
(c)
(a)
(6 and)
#t
#t
#t
#t
#f
#f
(apples peaches pumpkins pie)
((apples peaches) (pumpkin pie) (apples peaches))
((apples peaches) (pumpkin pie))
((4 3) (4 2) (7 6) (6 2) (3 4))
#f
#t
#f
((a 8) (pie pumpkin) (sick got))
((a 8) (pie pumpkin) (sick got))
#f
#t
#f
#f
#t
#f
#t

