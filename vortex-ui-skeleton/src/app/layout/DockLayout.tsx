import 'react-mosaic-component/react-mosaic-component.css'
import React, { useMemo, useState } from 'react'
import { Mosaic, MosaicWindow, getLeaves, MosaicNode } from 'react-mosaic-component'
import { GraphPanel } from '../panels/GraphPanel'
import { InspectorPanel } from '../panels/InspectorPanel'
import { LibraryPanel } from '../panels/LibraryPanel'
import { ConsolePanel } from '../panels/ConsolePanel'

type PanelId = 'graph' | 'inspector' | 'library' | 'console'

const DEFAULT_TREE: MosaicNode<PanelId> = {
  direction: 'row',
  first: 'library',
  second: {
    direction: 'column',
    first: 'graph',
    second: {
      direction: 'row',
      first: 'console',
      second: 'inspector',
      splitPercentage: 60
    },
    splitPercentage: 70
  },
  splitPercentage: 20
}

export function DockLayout() {
  const [tree, setTree] = useState<MosaicNode<PanelId> | null>(DEFAULT_TREE)

  const TITLE: Record<PanelId,string> = {
    graph: 'Graph',
    inspector: 'Inspector',
    library: 'Node Library',
    console: 'Console',
  }

  const RENDER = useMemo(() => (id: PanelId) => {
    switch (id) {
      case 'graph': return <GraphPanel />
      case 'inspector': return <InspectorPanel />
      case 'library': return <LibraryPanel />
      case 'console': return <ConsolePanel />
    }
  }, [])

  return (
    <div className="w-screen h-screen flex flex-col">
      <header className="h-10 border-b border-ui-border bg-ui-panel flex items-center px-3 justify-between">
        <div className="flex items-center gap-3">
          <strong>Vortex</strong>
          <span className="opacity-70 text-sm">File</span>
          <span className="opacity-70 text-sm">Edit</span>
          <span className="opacity-70 text-sm">View</span>
          <span className="opacity-70 text-sm">Window</span>
          <span className="opacity-70 text-sm">Help</span>
        </div>
        <div className="text-xs opacity-70">FPS: 60 • GPU: DX12</div>
      </header>
      <div className="flex-1">
        <Mosaic<PanelId>
          renderTile={(id, path) => (
            <MosaicWindow<PanelId> path={path} createNode={() => 'graph'} title={TITLE[id]}>
              <div className="w-full h-full bg-ui-panel border border-ui-border rounded-lg overflow-hidden">
                {RENDER(id)}
              </div>
            </MosaicWindow>
          )}
          value={tree}
          onChange={setTree}
          className="h-full"
        />
      </div>
      <footer className="h-6 border-t border-ui-border px-2 text-xs flex items-center justify-between opacity-70">
        <div>Status: Ready</div>
        <div>Autosave: ON</div>
      </footer>
    </div>
  )
}