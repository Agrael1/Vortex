import React from 'react'
import ReactFlow, { Background, Controls, MiniMap } from '@xyflow/react'
import '@xyflow/react/dist/style.css'

const initialNodes = [
  { id: '1', position: { x: 100, y: 100 }, data: { label: 'StreamInput' }, type: 'input' },
  { id: '2', position: { x: 380, y: 100 }, data: { label: 'Blend' } },
  { id: '3', position: { x: 660, y: 100 }, data: { label: 'WindowOutput' }, type: 'output' },
]
const initialEdges = [
  { id: 'e1', source: '1', target: '2' },
  { id: 'e2', source: '2', target: '3' },
]

export function GraphPanel() {
  return (
    <div className="w-full h-full">
      <ReactFlow defaultNodes={initialNodes} defaultEdges={initialEdges} fitView>
        <Background />
        <MiniMap />
        <Controls />
      </ReactFlow>
    </div>
  )
}