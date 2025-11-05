import React, { useEffect, useState } from 'react'
import { engine } from '../services_mirror'

type Entry = { level: 'info'|'warn'|'error'; message: string; time: number }

export function ConsolePanel() {
  const [entries, setEntries] = useState<Entry[]>([])

  useEffect(() => {
    const off = engine.on('log', (e: Entry) => setEntries((prev)=>[...prev, e]))
    // push some mock lines
    setTimeout(()=>engine.emitLog({ level:'info', message:'Engine ready', time: Date.now() }), 300)
    setTimeout(()=>engine.emitLog({ level:'info', message:'Graph compiled', time: Date.now() }), 800)
    return () => { off?.() }
  }, [])

  return (
    <div className="w-full h-full p-2 text-xs font-mono overflow-auto">
      {entries.map((e, i) => (
        <div key={i} className={e.level === 'error' ? 'text-red-400' : e.level === 'warn' ? 'text-yellow-300' : 'text-slate-200'}>
          {new Date(e.time).toLocaleTimeString()} — {e.message}
        </div>
      ))}
    </div>
  )
}