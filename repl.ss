(define _debug-compile
    (lambda (expr)
        (let ((result (_compile expr)))
            (begin
                (display "// parsed expression: " stdout)
                (display expr stdout)
                (newline stdout)
                (display "// compiled expression: " stdout)
                (display result stdout)
                (newline stdout)
                result))))

(define _compiler_strategy
    (if _arg-debug-compiler
        _debug-compile
        _compile))

(define _repl
    (lambda (fh)
        (let ((expr (read fh)))
            (if (eof-object? expr)
                '()
                (let ((result (eval (_compiler_strategy expr) _top_env)))
                    (begin 
                        (if (_nil? result)
                            '()
                            (begin
                                (write result stdout)
                                (newline stdout) ))
                    (_repl fh)) )))))


(_repl stdin)
