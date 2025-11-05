import React from 'react'

export function InspectorPanel() {
  return (
    <div className="w-full h-full p-3">
      <div className="text-sm uppercase opacity-70 mb-2">Inspector</div>
      <div className="space-y-2 text-sm">
        <div className="opacity-60">Select a node to edit properties</div>
        <div className="grid grid-cols-2 gap-2">
          <label className="opacity-70">Opacity</label>
          <input type="range" min={0} max={1} step={0.01} defaultValue={0.7} />
          <label className="opacity-70">Mode</label>
          <select className="bg-black/20 border border-ui-border rounded px-2 py-1">
            <option>add</option>
            <option>multiply</option>
            <option>screen</option>
          </select>
        </div>
      </div>
    </div>
  )
}