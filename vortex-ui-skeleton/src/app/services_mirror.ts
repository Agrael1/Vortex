type LogEntry = { level: 'info'|'warn'|'error'; message: string; time: number }
type Handler = (payload: any)=>void

class EngineMirror {
  private handlers: Record<string, Handler[]> = {}

  on(ev: string, cb: Handler) {
    this.handlers[ev] = this.handlers[ev] || []
    this.handlers[ev].push(cb)
    return () => {
      this.handlers[ev] = (this.handlers[ev] || []).filter(h => h !== cb)
    }
  }

  emit(ev: string, payload: any) {
    (this.handlers[ev] || []).forEach(h => h(payload))
  }

  emitLog(entry: LogEntry) {
    this.emit('log', entry)
  }
}

export const engine = new EngineMirror()