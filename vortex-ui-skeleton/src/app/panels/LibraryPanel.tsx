import React from 'react'

const groups: Record<string, string[]> = {
  Inputs: ['StreamInput','ImageInput','NDIInput'],
  Filters: ['Blend','ColorCorrect','Blur'],
  Output: ['NDIOutput','WindowOutput','FileWriter'],
  Utility: ['Switch','Timer','Constant'],
}

export function LibraryPanel() {
  return (
    <div className="w-full h-full p-3">
      <div className="text-sm uppercase opacity-70 mb-2">Node Library</div>
      <div className="space-y-3">
        {Object.entries(groups).map(([g, items]) => (
          <div key={g}>
            <div className="text-xs uppercase opacity-60 mb-1">{g}</div>
            <div className="grid grid-cols-2 gap-2">
              {items.map((n) => (
                <button key={n} className="text-left px-2 py-1 rounded bg-black/20 border border-ui-border hover:border-ui-accent">
                  {n}
                </button>
              ))}
            </div>
          </div>
        ))}
      </div>
    </div>
  )
}