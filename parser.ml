open SmallCTypes
open Utils
open TokenTypes

(*Version 2*)

(* Parsing helpers (you don't need to modify these) *)

(* Return types for parse_stmt and parse_expr *)
type stmt_result = token list * stmt
type expr_result = token list * expr

(* Return the next token in the token list, throwing an error if the list is empty *)
let lookahead (toks : token list) : token =
  match toks with
  | [] -> raise (InvalidInputException "No more tokens")
  | h::_ -> h

(* Matches the next token in the list, throwing an error if it doesn't match the given token *)
let match_token (toks : token list) (tok : token) : token list =
  match toks with
  | [] -> raise (InvalidInputException(string_of_token tok))
  | h::t when h = tok -> t
  | h::_ -> raise (InvalidInputException(
      Printf.sprintf "Expected %s from input %s, got %s"
        (string_of_token tok)
        (string_of_list string_of_token toks)
        (string_of_token h)
    ))

(* Parsing (TODO: implement your code below) *)

let rec parse_expr toks : expr_result =
    let (toks2, expr) = parse_OrExpr toks in
    (toks2, expr)

    and parse_OrExpr toks : expr_result =
        let (toks2, expr1) = parse_AndExpr toks in
        match (lookahead toks2) with
        | Tok_Or -> let toks3 = match_token toks2 Tok_Or in
                    let (toks4, expr2) = parse_OrExpr toks3 in
                    (toks4, Or(expr1, expr2))
        | _ -> (toks2, expr1)

    and parse_AndExpr toks : expr_result =
        let (toks2, expr1) = parse_EqualityExpr toks in
        match (lookahead toks2) with
        | Tok_And -> let toks3 = match_token toks2 Tok_And in
                     let (toks4, expr2) = parse_AndExpr toks3 in
                     (toks4, And(expr1, expr2))
        | _ -> (toks2, expr1)

    and parse_EqualityExpr toks : expr_result =
        let (toks2, expr1) = parse_RelationalExpr toks in
        match (lookahead toks2) with
        | Tok_Equal -> let toks3 = match_token toks2 Tok_Equal in
                       let (toks4, expr2) = parse_EqualityExpr toks3 in
                       (toks4, Equal(expr1, expr2))
        | Tok_NotEqual -> let toks3 = match_token toks2 Tok_NotEqual in
                          let (toks4, expr2) = parse_EqualityExpr toks3 in
                          (toks4, NotEqual(expr1, expr2))
        | _ -> (toks2, expr1)

    and parse_RelationalExpr toks : expr_result =
        let (toks2, expr1) = parse_AdditiveExpr toks in
        match (lookahead toks2) with
        | Tok_Greater -> let toks3 = match_token toks2 Tok_Greater in
                         let (toks4, expr2) = parse_RelationalExpr toks3 in
                         (toks4, Greater(expr1, expr2))
        | Tok_Less -> let toks3 = match_token toks2 Tok_Less in
                      let (toks4, expr2) = parse_RelationalExpr toks3 in
                      (toks4, Less(expr1, expr2))
        | Tok_GreaterEqual -> let toks3 = match_token toks2 Tok_GreaterEqual in
                              let (toks4, expr2) = parse_RelationalExpr toks3 in
                              (toks4, GreaterEqual(expr1, expr2))
        | Tok_LessEqual -> let toks3 = match_token toks2 Tok_LessEqual in
                           let (toks4, expr2) = parse_RelationalExpr toks3 in
                           (toks4, LessEqual(expr1, expr2))
        | _ -> (toks2, expr1)

    and parse_AdditiveExpr toks : expr_result =
        let (toks2, expr1) = parse_MultiplicativeExpr toks in
        match (lookahead toks2) with
        | Tok_Add -> let toks3 = match_token toks2 Tok_Add in
                     let (toks4, expr2) = parse_AdditiveExpr toks3 in
                     (toks4, Add(expr1, expr2))
        | Tok_Sub -> let toks3 = match_token toks2 Tok_Sub in
                     let (toks4, expr2) = parse_AdditiveExpr toks3 in
                     (toks4, Sub(expr1, expr2))
        | _ -> (toks2, expr1)

    and parse_MultiplicativeExpr toks : expr_result =
        let (toks2, expr1) = parse_PowerExpr toks in
        match (lookahead toks2) with
        | Tok_Mult -> let toks3 = match_token toks2 Tok_Mult in
                      let (toks4, expr2) = parse_MultiplicativeExpr toks3 in
                      (toks4, Mult(expr1, expr2))
        | Tok_Div -> let toks3 = match_token toks2 Tok_Div in
                     let (toks4, expr2) = parse_MultiplicativeExpr toks3 in
                     (toks4, Div(expr1, expr2))
        | _ -> (toks2, expr1)

    and parse_PowerExpr toks : expr_result =
        let (toks2, expr1) = parse_UnaryExpr toks in
        match (lookahead toks2) with
        | Tok_Pow -> let toks3 = match_token toks2 Tok_Pow in
                     let (toks4, expr2) = parse_PowerExpr toks3 in
                     (toks4, Pow(expr1, expr2))
        | _ -> (toks2, expr1)

    and parse_UnaryExpr toks : expr_result =
        match (lookahead toks) with
        | Tok_Not -> let toks2 = match_token toks Tok_Not in
                     let (toks3, expr1) = parse_UnaryExpr toks2 in
                     (toks3, Not(expr1))
        | _ -> parse_PrimaryExpr toks

    and parse_PrimaryExpr toks : expr_result =
        match (lookahead toks) with
        | Tok_Int(i) -> let toks2 = match_token toks (Tok_Int(i)) in
                        (toks2, Int(i))
        | Tok_Bool(b) -> let toks2 = match_token toks (Tok_Bool(b)) in
                          (toks2, Bool(b))
        | Tok_ID(s) -> let toks2 = match_token toks (Tok_ID(s)) in
                        (toks2, ID(s))
        | Tok_LParen -> let toks2 = match_token toks Tok_LParen in
                        let (toks3, expr1) = parse_expr toks2 in
                        let toks4 = match_token toks3 Tok_RParen in
                        (toks4, expr1)
        | _ -> raise (InvalidInputException (string_of_token (lookahead toks)))

let rec parse_stmt toks : stmt_result =
    if (lookahead toks) != EOF then
        match (lookahead toks) with
        | Tok_Int_Type -> let (toks2, stmt1) = parse_DeclareStmt toks in
                          let (toks3, stmt2) = parse_stmt toks2 in
                          (toks3, Seq(stmt1, stmt2))
        | Tok_Bool_Type -> let (toks2, stmt1) = parse_DeclareStmt toks in
                           let (toks3, stmt2) = parse_stmt toks2 in
                           (toks3, Seq(stmt1, stmt2))
        | Tok_ID(i) -> let (toks2, stmt1) = parse_AssignStmt toks in
                       let (toks3, stmt2) = parse_stmt toks2 in
                       (toks3, Seq(stmt1, stmt2))
        | Tok_Print -> let (toks2, stmt1) = parse_PrintStmt toks in
                       let (toks3, stmt2) = parse_stmt toks2 in
                       (toks3, Seq(stmt1, stmt2))
        | Tok_If -> let (toks2, stmt1) = parse_IfStmt toks in
                    let (toks3, stmt2) = parse_stmt toks2 in
                    (toks3, Seq(stmt1, stmt2))
        | Tok_For -> let (toks2, stmt1) = parse_ForStmt toks in
                     let (toks3, stmt2) = parse_stmt toks2 in
                     (toks3, Seq(stmt1, stmt2))
        | Tok_While -> let (toks2, stmt1) = parse_WhileStmt toks in
                       let (toks3, stmt2) = parse_stmt toks2 in
                       (toks3, Seq(stmt1, stmt2))
        | _ -> (toks, NoOp)

    else ([EOF], NoOp)

    and parse_DeclareStmt toks : stmt_result =
        if (lookahead toks) = Tok_Int_Type then
            let toks2 = match_token toks Tok_Int_Type in
            match (lookahead toks2) with
            | Tok_ID(i) -> let toks3 = match_token toks2 (Tok_ID(i)) in
                           let toks4 = match_token toks3 Tok_Semi in
                           (toks4, Declare(Int_Type, i))
            | _ -> raise (InvalidInputException "No ID found in parse_DeclareStmt")

        else
            let toks2 = match_token toks Tok_Bool_Type in
            match (lookahead toks2) with
            | Tok_ID(i) -> let toks3 = match_token toks2 (Tok_ID(i)) in
                           let toks4 = match_token toks3 Tok_Semi in
                           (toks4, Declare(Bool_Type, i))
            | _ -> raise (InvalidInputException "No ID found in parse_DeclareStmt")

    and parse_AssignStmt toks : stmt_result =
        match (lookahead toks) with
        | Tok_ID(i) -> let toks2 = match_token toks (Tok_ID(i)) in
                       let toks3 = match_token toks2 Tok_Assign in
                       let (toks4, expr1) = parse_expr toks3 in
                       let toks5 = match_token toks4 Tok_Semi in
                       (toks5, Assign(i, expr1))
        | _ -> raise (InvalidInputException "No ID found in parse_AssignStmt")

    and parse_PrintStmt toks : stmt_result =
        let toks2 = match_token toks Tok_Print in
        let toks3 = match_token toks2 Tok_LParen in
        let (toks4, expr1) = parse_expr toks3 in
        let toks5 = match_token toks4 Tok_RParen in
        let toks6 = match_token toks5 Tok_Semi in
        (toks6, Print(expr1))

    and parse_IfStmt toks : stmt_result =
        let toks2 = match_token toks Tok_If in
        let toks3 = match_token toks2 Tok_LParen in
        let (toks4, expr1) = parse_expr toks3 in
        let toks5 = match_token toks4 Tok_RParen in
        let toks6 = match_token toks5 Tok_LBrace in
        let (toks7, stmt1) = parse_stmt toks6 in
        let toks8 = match_token toks7 Tok_RBrace in
        let (toks9, stmt2) = parse_ElseBranch toks8 in
        (toks9, If(expr1, stmt1, stmt2))

    and parse_ElseBranch toks : stmt_result =
        match (lookahead toks) with
        | Tok_Else -> let toks2 = match_token toks Tok_Else in
                      let toks3 = match_token toks2 Tok_LBrace in
                      let (toks4, stmt1) = parse_stmt toks3 in
                      let toks5 = match_token toks4 Tok_RBrace in
                      (toks5, stmt1)
        | _ -> (toks, NoOp)

    and parse_ForStmt toks : stmt_result =
        let toks2 = match_token toks Tok_For in
        let toks3 = match_token toks2 Tok_LParen in
        match (lookahead toks3) with
        | Tok_ID(i) -> let toks4 = match_token toks3 (Tok_ID(i)) in
                       let toks5 = match_token toks4 Tok_From in
                       let (toks6, expr1) = parse_expr toks5 in
                       let toks7 = match_token toks6 Tok_To in
                       let (toks8, expr2) = parse_expr toks7 in
                       let toks9 = match_token toks8 Tok_RParen in
                       let toks10 = match_token toks9 Tok_LBrace in
                       let (toks11, stmt1) = parse_stmt toks10 in
                       let toks12 = match_token toks11 Tok_RBrace in
                       (toks12, For(i, expr1, expr2, stmt1))
        | _ -> raise (InvalidInputException "No ID in parse_ForStmt")

    and parse_WhileStmt toks : stmt_result =
        let toks2 = match_token toks Tok_While in
        let toks3 = match_token toks2 Tok_LParen in
        let (toks4, expr1) = parse_expr toks3 in
        let toks5 = match_token toks4 Tok_RParen in
        let toks6 = match_token toks5 Tok_LBrace in
        let (toks7, stmt1) = parse_stmt toks6 in
        let toks8 = match_token toks7 Tok_RBrace in
        (toks8, While(expr1, stmt1))

let parse_main toks : stmt =
    if (lookahead toks) = Tok_Int_Type then
        let toks2 = match_token toks Tok_Int_Type in
        let toks3 = match_token toks2 Tok_Main in
        let toks4 = match_token toks3 Tok_LParen in
        let toks5 = match_token toks4 Tok_RParen in
        let toks6 = match_token toks5 Tok_LBrace in
        let (toks7, stmt1) = parse_stmt toks6 in
        let toks8 = match_token toks7 Tok_RBrace in
        if toks8 <> [EOF] then
            raise (InvalidInputException "Remaining tokens")
        else
            stmt1
    else
        raise (InvalidInputException "Not a main function")
