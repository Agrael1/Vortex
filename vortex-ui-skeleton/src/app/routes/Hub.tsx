import { useEffect, useState } from 'react'
import { useNavigate } from 'react-router-dom'
import { engine } from '../../services/ipc/cefBridge'

type Recent = { name: string; path: string; last: string }

export function Hub() {
  const nav = useNavigate()
  const [recents, setRecents] = useState<Recent[]>([])

  useEffect(() => {
    engine.listRecent().then(setRecents)
  }, [])

  return (
    <div className="w-screen h-screen flex flex-col bg-ui-bg">
      <header className="h-12 border-b border-ui-border flex items-center px-4 justify-between">
        <div className="font-semibold">Vortex Hub</div>
        <div className="flex gap-2">
          <button className="px-3 py-1 rounded bg-ui-panel border border-ui-border" onClick={()=>nav('/editor')}>New Project</button>
          <button className="px-3 py-1 rounded bg-ui-panel border border-ui-border" onClick={()=>nav('/editor')}>Open…</button>
        </div>
      </header>

      <main className="flex-1 grid grid-cols-12 gap-4 p-4">
        <section className="col-span-8">
          <h2 className="text-sm uppercase opacity-70 mb-2">Recent Projects</h2>
          <div className="grid grid-cols-3 gap-3">
            {recents.map((r) => (
              <div key={r.path} className="rounded-xl border border-ui-border bg-ui-panel p-3 hover:border-ui-accent transition-colors cursor-pointer"
                   onDoubleClick={()=>nav('/editor')}>
                <div className="font-medium">{r.name}</div>
                <div className="text-xs opacity-60">{r.path}</div>
                <div className="text-xs opacity-60 mt-1">Last: {new Date(r.last).toLocaleString()}</div>
              </div>
            ))}
            {recents.length === 0 && (
              <div className="text-sm opacity-60">No recent projects yet.</div>
            )}
          </div>
        </section>

        <aside className="col-span-4">
          <h2 className="text-sm uppercase opacity-70 mb-2">Templates</h2>
          <div className="grid gap-3">
            {['Blank','Live Stream','NDI Bridge','Video Processor'].map(t => (
              <div key={t} className="rounded-xl border border-ui-border bg-ui-panel p-3 flex items-center justify-between">
                <div>{t}</div>
                <button className="px-3 py-1 rounded bg-black/20 border border-ui-border" onClick={()=>nav('/editor')}>Create</button>
              </div>
            ))}
          </div>
        </aside>
      </main>
    </div>
  )
}