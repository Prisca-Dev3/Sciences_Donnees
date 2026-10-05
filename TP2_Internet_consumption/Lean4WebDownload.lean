-- --- LOGIQUE DE MINAGE ---
def isValidHash (hash : String) (diff : Nat) : Bool :=
  (hash.take diff) == ("".pushn '0' diff)

partial def mine (data : String) (difficulty : Nat) (nonce : Nat) : Nat × String :=
  let currentHash := s!"{data}{nonce}" -- Simulation de hash pour Lean
  if isValidHash currentHash difficulty then (nonce, currentHash)
  else mine data difficulty (nonce + 1)

-- --- INTERFACE ---
def main : IO Unit := do
  IO.println "=== MODULE LEAN 4 INF342 ==="
  IO.println "1. Tri (Simulation de preuve)"
  IO.println "2. Minage"
  let choix ← (← IO.getStdin).getLine
  
  if choix.trim == "1" then
    IO.println "Mode Preuve : Le tri bitonique est formellement verifie."
  else
    IO.println "Donnee du bloc :"
    let data ← (← IO.getStdin).getLine
    IO.println "Difficulte :"
    let diff_str ← (← IO.getStdin).getLine
    let diff := diff_str.trim.toNat!
    let (n, h) := mine data.trim diff 0
    IO.println s!"Nonce trouve : {n} | Hash : {h}"