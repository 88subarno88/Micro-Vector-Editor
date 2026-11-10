// This user's undo and redo: only their own edits, never anyone else's. Undoing doesn't rewind the
// document; it makes a new edit, the step's reverse, which the server orders after everything else like
// any other edit. If the reverse no longer applies, as when someone else deleted the shape, the step can't
// be undone.

export interface Step {
  made: object[] // the ops that made this change, as sent: if the server refuses any of them, the step goes
  reverse: object // the op that takes it back
  group?: object // the drag it came from, if any: a drag is undone in one step
}

type Move = { kind: 'move'; id: string; dx: number; dy: number }
const isMove = (op: object): op is Move => (op as { kind?: string }).kind === 'move'

export class History {
  private undos: Step[] = []
  private redos: Step[] = []

  get canUndo() {
    return this.undos.length > 0
  }

  get canRedo() {
    return this.redos.length > 0
  }

  // A new edit, with the op that reverses it. Moves from the same drag (`group`) join one step, whose
  // reverse is a single move by their total, so undoing a drag sends one op, not dozens. A new edit ends
  // any redoing.
  record(op: object, reverse: object, group?: object) {
    this.redos = []
    const last = this.undos.at(-1)
    if (group && last?.group === group && isMove(last.reverse) && isMove(reverse)) {
      last.made.push(op)
      last.reverse = { ...last.reverse, dx: last.reverse.dx + reverse.dx, dy: last.reverse.dy + reverse.dy }
      return
    }
    this.undos.push({ made: [op], reverse, group })
  }

  takeUndo(): Step | undefined {
    return this.undos.pop()
  }

  takeRedo(): Step | undefined {
    return this.redos.pop()
  }

  // Once a step has been undone or redone, the step that would take that back again.
  pushUndo(step: Step) {
    this.undos.push(step)
  }

  pushRedo(step: Step) {
    this.redos.push(step)
  }

  // The server refused `op`: the step it belongs to never happened, so it can't be undone or redone.
  forget(op: object) {
    const keep = (step: Step) => !step.made.includes(op)
    this.undos = this.undos.filter(keep)
    this.redos = this.redos.filter(keep)
  }
}
