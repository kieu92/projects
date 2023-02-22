open SmallCTypes
open EvalUtils
open TokenTypes


exception TypeError of string
exception DeclareError of string
exception DivByZeroError

let extend env x v = (x, v)::env

let rec lookup env x = match env with
| [] -> raise (DeclareError "lookup: undefined")
| (y, v)::env' -> if x = y then v else lookup env' x

let rec is_bool b = match b with
| Bool_Val(v) -> true
| _ -> false

let rec is_int i = match i with
| Int_Val(i) -> true
| _ -> false

let rec check_str env n = match env with
| [] -> false
| (x, v)::t -> if n = x then true else check_str t n

let getNum n = match n with
| Int_Val(i) -> i
| _ -> raise (TypeError "getNum: not a num")

let getFloat n = match n with
| Int_Val(i) -> float_of_int i
| _ -> raise (TypeError "getFloat: not a num")

let getBool n = match n with
| Bool_Val(b) -> b
| _ -> raise (TypeError "getBool: not a bool")

let equalHelper n1 n2 = match n1 with
| Int_Val(i) -> i = getNum(n2)
| Bool_Val(b) -> b = getBool(n2)

let notequalHelper n1 n2 = match n1 with
| Int_Val(i) -> i != getNum(n2)
| Bool_Val(b) -> b != getBool(n2)

let rec eval_expr env t = match t with
| Int(i) -> Int_Val(i)
| Bool(b) -> Bool_Val(b)
| ID(i) -> lookup env i

| Add(e1, e2) -> let n1 = eval_expr env e1 in
                 let n2 = eval_expr env e2 in
                 let i1 = getNum(n1) in
                 let i2 = getNum(n2) in
                 let n3 = i1 + i2 in
                 Int_Val(n3)

| Sub(e1, e2) -> let n1 = eval_expr env e1 in
                 let n2 = eval_expr env e2 in
                 let i1 = getNum(n1) in
                 let i2 = getNum(n2) in
                 let n3 = i1 - i2 in
                 Int_Val(n3)

| Mult(e1, e2) -> let n1 = eval_expr env e1 in
                  let n2 = eval_expr env e2 in
                  let i1 = getNum(n1) in
                  let i2 = getNum(n2) in
                  let n3 = i1 * i2 in
                  Int_Val(n3)

| Div(e1, e2) -> let n2 = eval_expr env e2 in
                 if getNum(n2) = 0 then
                    raise DivByZeroError
                 else
                    let n1 = eval_expr env e1 in
                    let i1 = getNum(n1) in
                    let i2 = getNum(n2) in
                    let n3 = i1 / i2 in
                    Int_Val(n3)

| Pow(e1, e2) -> let n1 = eval_expr env e1 in
                 let n2 = eval_expr env e2 in
                 let f1 = getFloat(n1) in
                 let f2 = getFloat(n2) in
                 let n3 = Float.pow f1 f2 in
                 Int_Val(int_of_float (Float.floor n3))

| Or(e1, e2) -> let n1 = eval_expr env e1 in
                let n2 = eval_expr env e2 in
                let b1 = getBool(n1) in
                let b2 = getBool(n2) in
                let n3 = b1 || b2 in
                Bool_Val(n3)

| And(e1, e2) -> let n1 = eval_expr env e1 in
                 let n2 = eval_expr env e2 in
                 let b1 = getBool(n1) in
                 let b2 = getBool(n2) in
                 let n3 = b1 && b2 in
                 Bool_Val(n3)

| Not(e1) -> let n1 = eval_expr env e1 in
             let n2 = Bool.not(getBool n1) in
             Bool_Val(n2)

| Greater(e1, e2) -> let n1 = eval_expr env e1 in
                     let n2 = eval_expr env e2 in
                     let i1 = getNum(n1) in
                     let i2 = getNum(n2) in
                     let n3 = i1 > i2 in
                     Bool_Val(n3)

| Less(e1, e2) -> let n1 = eval_expr env e1 in
                  let n2 = eval_expr env e2 in
                  let i1 = getNum(n1) in
                  let i2 = getNum(n2) in
                  let n3 = i1 < i2 in
                  Bool_Val(n3)

| GreaterEqual(e1, e2) -> let n1 = eval_expr env e1 in
                          let n2 = eval_expr env e2 in
                          let i1 = getNum(n1) in
                          let i2 = getNum(n2) in
                          let n3 = i1 >= i2 in
                          Bool_Val(n3)

| LessEqual(e1, e2) -> let n1 = eval_expr env e1 in
                       let n2 = eval_expr env e2 in
                       let i1 = getNum(n1) in
                       let i2 = getNum(n2) in
                       let n3 = i1 <= i2 in
                       Bool_Val(n3)

| Equal(e1, e2) -> let n1 = eval_expr env e1 in
                   let n2 = eval_expr env e2 in
                   Bool_Val(equalHelper n1 n2)

| NotEqual(e1, e2) -> let n1 = eval_expr env e1 in
                      let n2 = eval_expr env e2 in
                      Bool_Val(notequalHelper n1 n2)

let rec eval_stmt env s = match s with
| NoOp -> env

| Seq(s1,s2) -> let env1 = eval_stmt env s1 in
                let env2 = eval_stmt env1 s2 in
                env2

| Declare(d,s) -> declare_helper env d s

| Assign(s,e) -> assign_helper env s e

| If(e1,s1,s2) -> let v1 = eval_expr env e1 in
                  if v1 = Bool_Val(true) then
                      let env' = eval_stmt env s1 in
                      env'
                  else if v1 = Bool_Val(false) then
                      let env' = eval_stmt env s2 in
                      env'
                  else
                      raise (TypeError "If: not a bool")

| While(e1,s1) -> while_helper env e1 s1

| For(str,e1,e2,s1) -> for_helper env str e1 e2 s1

| Print(e1) -> let v1 = eval_expr env e1 in
               if (is_bool v1) then
                   (
                       print_output_bool (getBool v1);
                       print_output_newline();
                       env
                    )
               else if (is_int v1) then
                   (
                       print_output_int (getNum v1);
                       print_output_newline();
                       env
                    )
               else
                   failwith "Print: not bool or num"

    and declare_helper env d s =
        if (check_str env s) == false then
            if d = Int_Type then
                let env' = extend env s (Int_Val(0)) in env'
            else
                let env' = extend env s (Bool_Val(false)) in env'
        else raise (DeclareError "Declare: var already declared")

    and assign_helper env s e =
        let v1 = lookup env s in
        let v2 = eval_expr env e in
        if is_bool(v1) && is_bool(v2) then
            List.fold_left (fun a (str, v) -> if s = str then (str, v2)::a else (str, v)::a) [] env
        else if is_int(v1) && is_int(v2) then
            List.fold_left (fun a (str, v) -> if s = str then (str, v2)::a else (str, v)::a) [] env
        else raise (TypeError "Assign: not int or bool")

    and while_helper env e1 s1 =
        let v1 = eval_expr env e1 in
        if v1 = Bool_Val(true) then
            let env' = eval_stmt env s1 in
            while_helper env' e1 s1
        else if v1 = Bool_Val(false) then
            env
        else
            raise (TypeError "While: not a bool")

    and for_helper env str e1 e2 s1 =
        let n1 = eval_expr env e1 in
        let n2 = eval_expr env e2 in
        if getNum(n1) <= getNum(n2) then
            let env1 = assign_helper env str (Int(getNum n1)) in
            let env2 = eval_stmt env1 s1 in
            let env3 = assign_helper env2 str (Int(getNum(lookup env2 str) + 1)) in
            for_helper env3 str (Int(getNum(lookup env2 str) + 1)) e2 s1
        else
            let env1 = assign_helper env str (Int(getNum n1)) in
            env1

