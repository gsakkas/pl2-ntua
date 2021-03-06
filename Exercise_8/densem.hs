{- Author:    Georgios Sakkas
 - Compliler: Glasgow Haskell Compiler, Version 7.10.3
 -
 - Compile with: ghc densem.hs
 - Run with:     ./densem < [input-file]
 -
 - OR
 -
 - Run with:      ghci densem.hs
 - Test examples: > run ex[0-3]
 -
 - Both ways return the value of result variable
 -}

{-# OPTIONS_GHC -O2 -optc-O2 #-}

import Data.Char
import Text.Read
import Text.Read.Lex

-- Syntax

type Var = String

data C = Cskip | Cexpr N | Cseq C C | Cfor N C | Cif B C C | Cwhile B C
data N = Nzero | Nsucc N | Npred N | Nvar Var
               | Nassign Var N | Ninc Var | Ndec Var
data B = Btrue | Bfalse | Blt N N | Beq N N | Bnot B


-- Semantic domains

type S = Var -> Integer


-- Pretty-printing

instance Show N where
  showsPrec p Nzero = ("0" ++)
  showsPrec p (Nsucc n) = ("succ " ++) . showsPrec 1 n
  showsPrec p (Npred n) = ("pred " ++) . showsPrec 1 n
  showsPrec p (Nvar x) = (x ++)
  showsPrec p (Nassign x n) =
    showParen (p > 0) $
    (x ++) . (" := " ++) . showsPrec 0 n
  showsPrec p (Ninc x) = (x ++) . ("++" ++)
  showsPrec p (Ndec x) = (x ++) . ("--" ++)

instance Show B where
  showsPrec p Btrue = ("true" ++)
  showsPrec p Bfalse = ("false" ++)
  showsPrec p (Blt n1 n2) = showsPrec 0 n1 . (" < " ++) . showsPrec 0 n2
  showsPrec p (Beq n1 n2) = showsPrec 0 n1 . (" = " ++) . showsPrec 0 n2
  showsPrec p (Bnot b) = ("not " ++) . showsPrec 0 b

instance Show C where
  showsPrec p Cskip = ("skip" ++)
  showsPrec p (Cexpr n) = showsPrec 0 n
  showsPrec p (Cseq c1 c2) =
    showParen (p > 0) $
    showsPrec 1 c1 . ("; " ++) . showsPrec 0 c2
  showsPrec p (Cfor n c) =
    ("for " ++) . showsPrec 0 n . (" do " ++) . showsPrec 1 c
  showsPrec p (Cif b c1 c2) =
    ("if " ++) . showsPrec 0 b . (" then " ++) . showsPrec 1 c1 .
                                 (" else " ++) . showsPrec 1 c2
  showsPrec p (Cwhile b c) =
    ("while " ++) . showsPrec 0 b . (" do " ++) . showsPrec 1 c


-- Parsing

isVar x = all isAlpha x && not (x `elem` keywords)
  where keywords = ["zero", "succ", "true", "not", "skip",
                    "for", "if", "then", "else", "while", "do"]

when True p = p
when False _ = fail "when failed"

instance Read N where
  readPrec = parens $
             (prec 2 $ do
                Number n <- lexP
                when (numberToInteger n == Just 0) $ do
                  return Nzero) <++
             (prec 1 $ do
                Ident "succ" <- lexP
                n <- readPrec
                return (Nsucc n)) <++
             (prec 1 $ do
                Ident "pred" <- lexP
                n <- readPrec
                return (Npred n)) <++
             (prec 0 $ do
                Ident x <- lexP
                when (isVar x) $ do
                  Symbol ":=" <- lexP
                  n <- readPrec
                  return (Nassign x n)) <++
             (prec 2 $ do
                Ident x <- lexP
                when (isVar x) $ do
                  Symbol "++" <- lexP
                  return (Ninc x)) <++
             (prec 2 $ do
                Ident x <- lexP
                when (isVar x) $ do
                  Symbol "--" <- lexP
                  return (Ndec x)) <++
             (prec 2 $ do
                Ident x <- lexP
                when (isVar x) $ do
                  return (Nvar x)) 

instance Read B where
  readPrec = parens $
             (prec 2 $ do
                Ident "true" <- lexP
                return Btrue) <++
             (prec 2 $ do
                Ident "false" <- lexP
                return Btrue) <++
             (prec 0 $ do
                n1 <- readPrec
                Symbol "<" <- lexP
                n2 <- readPrec
                return (Blt n1 n2)) <++
             (prec 0 $ do
                n1 <- readPrec
                Symbol "=" <- lexP
                n2 <- readPrec
                return (Beq n1 n2)) <++
             (prec 0 $ do
                Ident "not" <- lexP
                b <- readPrec
                return (Bnot b))

instance Read C where
  readPrec = parens $
             (prec 0 $ do
                c1 <- step readPrec
                Punc ";" <- lexP
                c2 <- readPrec
                return (Cseq c1 c2)) <++
             (prec 1 $ do
                Ident "skip" <- lexP
                return Cskip) <++
             (prec 1 $ do
                n <- reset readPrec
                return (Cexpr n)) <++
             (prec 1 $ do
                Ident "if" <- lexP
                b <- reset readPrec
                Ident "then" <- lexP
                c1 <- readPrec
                Ident "else" <- lexP
                c2 <- readPrec
                return (Cif b c1 c2)) <++
             (prec 1 $ do
                Ident "for" <- lexP
                n <- reset readPrec
                Ident "do" <- lexP
                c <- readPrec
                return (Cfor n c)) <++
             (prec 1 $ do
                Ident "while" <- lexP
                b <- reset readPrec
                Ident "do" <- lexP
                c <- readPrec
                return (Cwhile b c))


-- Semantic functions

semC :: C -> S -> S
semC Cskip s = s
semC (Cexpr n) s = s'
  where (s', n') = semN n s
semC (Cseq c1 c2) s = semC c2 (semC c1 s)
semC (Cfor n c) s = expon n' (semC c) s'
  where (s', n') = semN n s
semC (Cif b c1 c2) s | b'        = semC c1 s'
                     | otherwise = semC c2 s'
  where (s', b') = semB b s
semC (Cwhile b c) s = fix bigF s
  where bigF f s | b'        = f (semC c s')
                 | otherwise = s'
          where (s', b') = semB b s

semN :: N -> S -> (S, Integer)
semN Nzero s = (s, 0)
semN (Nsucc n) s = (s', n' + 1)
  where (s', n') = semN n s
semN (Npred n) s = (s', n' - 1)
  where (s', n') = semN n s
semN (Nvar x) s = (s, s x)
semN (Nassign x n) s = (update s' x n', n')
  where (s', n') = semN n s
semN (Ninc x) s = (update s x n, n)
  where n = (s x) + 1
semN (Ndec x) s = (update s x n, n)
  where n = (s x) - 1

semB :: B -> S -> (S, Bool)
semB Btrue s = (s, True)
semB Bfalse s = (s, False)
semB (Blt n1 n2) s = (s'', n1' < n2')
  where (s', n1') = semN n1 s
        (s'', n2') = semN n2 s'
semB (Beq n1 n2) s = (s'', n1' == n2')
  where (s', n1') = semN n1 s
        (s'', n2') = semN n2 s'
semB (Bnot b) s = (s', not b')
  where (s', b') = semB b s


-- Auxiliary functions

fix f = f (fix f)

expon 0 f = id
expon n f = f . expon (n-1) f

update s x n y | x == y    = n
               | otherwise = s y

makeN 0 = Nzero
makeN n = Nsucc (makeN (n-1))


-- Initial state
s0 x = error ("not initialized variable " ++ x)


-- Run example and print result variable
run c = print (semC c s0 "result")


-- Examples

ex0 = Cexpr (Nassign "result" (makeN 42))

ex1 = Cseq (Cexpr (Nassign "result" Nzero))
           (Cfor (makeN 6) (
              Cfor (makeN 7) (
                Cexpr (Nassign "result" (Nsucc (Nvar "result"))))))

ex2 = Cseq (Cexpr (Nassign "x" (makeN 42)))
           (Cseq (Cexpr (Nassign "result" Nzero))
                 (Cwhile (Blt Nzero (Nvar "x"))
                         (Cseq (Cexpr (Nassign "x" (Npred (Nvar "x"))))
                               (Cexpr (Nassign "result" (Nsucc (Nvar "result")))))))

ex3 = Cseq (Cexpr (Nassign "result" Nzero))
           (Cfor (makeN 6) (
              Cfor (makeN 7) (
                Cexpr (Ndec "result"))))


-- Main function: parsing a statement and pretty-printing

main = do  input <- getContents
           let c :: C
               c = read input
           run c
