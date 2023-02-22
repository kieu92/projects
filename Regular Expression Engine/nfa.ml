open List
open Sets

(*********)
(* Types *)
(*********)

type ('q, 's) transition = 'q * 's option * 'q

type ('q, 's) nfa_t = {
  sigma: 's list;
  qs: 'q list;
  q0: 'q;
  fs: 'q list;
  delta: ('q, 's) transition list;
}

(***********)
(* Utility *)
(***********)

(* explode converts a string to a character list *)
let explode (s: string) : char list =
  let rec exp i l =
    if i < 0 then l else exp (i - 1) (s.[i] :: l)
  in
  exp (String.length s - 1) []

(****************)
(* Part 1: NFAs *)
(****************)

let rec move (nfa: ('q,'s) nfa_t) (qs: 'q list) (s: 's option) : 'q list =
    match qs with
    | [] -> []
    | h::t -> Sets.union (List.fold_left (fun a (x,y,z) -> if h = x && s = y
    then (Sets.union a [z]) else a) [] nfa.delta) (move nfa t s)

let rec e_closure (nfa: ('q,'s) nfa_t) (qs: 'q list) : 'q list =
    List.fold_left (fun a b -> let state = move nfa [b] None in
    if (Sets.subset state a) = false
        then e_closure nfa (Sets.union a state) else a) qs qs

(*******************************)
(* Part 2: Subset Construction *)
(*******************************)

let new_states (nfa: ('q,'s) nfa_t) (qs: 'q list) : 'q list list =
    List.map (fun a -> List.fold_left (fun b c -> let state = move nfa [c] (Some a) in
    if state = [] then b else Sets.union b (e_closure nfa state)) [] qs) nfa.sigma

let new_trans (nfa: ('q,'s) nfa_t) (qs: 'q list) : ('q list, 's) transition list =
    List.map (fun a -> List.fold_left (fun (x,y,z) c -> let state = move nfa [c] (Some a) in
    if state = [] then (x,y,z) else (qs,Some(a),(Sets.union z (e_closure nfa state)))) (qs,Some(a),[]) qs) nfa.sigma

let new_finals (nfa: ('q,'s) nfa_t) (qs: 'q list) : 'q list list =
    let is_final = List.fold_left (fun a b -> if (Sets.subset [b] qs) then true
    else a) false nfa.fs in
    if is_final then [qs] else []

let contain_lst lst llst =
    List.fold_left (fun a b -> if lst = b then true else a) false llst

let rec get_work nfa r lst =
    List.fold_left (fun a b -> if (contain_lst b a) = false && b != []
    then (get_work nfa (Sets.union a [b]) (new_states nfa b)) else a) r lst

let rec get_delta lst =
    List.fold_left (fun a (x,y,z) -> if z != [] then Sets.union a [(x,y,z)] else a) [] lst

let rec nfa_to_dfa_step (nfa: ('q,'s) nfa_t) (dfa: ('q list, 's) nfa_t)
    (work: 'q list list) : ('q list, 's) nfa_t =
    let lst = List.fold_left (fun a b -> Sets.union a (new_trans nfa b)) [] work in
    let new_delta = get_delta lst in
    let new_fs = List.fold_left (fun a b -> if (new_finals nfa b) != [] then Sets.union a (new_finals nfa b) else a) [] work in

    let new_dfa =
    {
        sigma = dfa.sigma;
        qs = dfa.qs;
        q0 = dfa.q0;
        fs = new_fs;
        delta = new_delta
    } in new_dfa

let nfa_to_dfa (nfa: ('q,'s) nfa_t) : ('q list, 's) nfa_t =
    let r0 = e_closure nfa [nfa.q0] in
    let work = get_work nfa [r0] (new_states nfa r0) in

    let dfa =
    {
        sigma = Sets.union nfa.sigma [];
        qs = work;
        q0 = r0;
        fs = [];
        delta = []
    } in nfa_to_dfa_step nfa dfa work

let accept (nfa: ('q,char) nfa_t) (s: string) : bool =
    let new_dfa = nfa_to_dfa nfa in
    let char_lst = explode s in
    let final = List.fold_left (fun a b -> move new_dfa a (Some b)) [new_dfa.q0] char_lst in
    if final = [] then false else (Sets.subset final new_dfa.fs)
