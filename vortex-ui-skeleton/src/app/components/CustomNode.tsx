import { memo } from 'react';
import { Handle, Position } from '@xyflow/react';

export interface CustomNodeData extends Record<string, unknown> {
  label: string;
  ptr: number;
  type?: string;
}

interface CustomNodeProps {
  data: CustomNodeData;
  selected?: boolean;
}

const NODE_TYPES = {
  ImageInput: { color: '#3b82f6', icon: '🖼️' },
  StreamInput: { color: '#8b5cf6', icon: '📹' },
  WindowOutput: { color: '#ef4444', icon: '🖥️' },
  NDIOutput: { color: '#f59e0b', icon: '📡' },
  Blend: { color: '#10b981', icon: '🎨' },
  Select: { color: '#6b7280', icon: '🔄' },
} as const;

export const CustomNode = memo(({ data, selected }: CustomNodeProps) => {
  const nodeType = data.type || data.label;
  const config = NODE_TYPES[nodeType as keyof typeof NODE_TYPES] || { 
    color: '#6b7280', 
    icon: '⚙️' 
  };
  
  return (
    <div 
      className={`
        relative px-4 py-3 rounded-lg border-2 transition-all duration-200
        min-w-[120px] max-w-[200px]
        ${selected 
          ? 'border-blue-400 shadow-lg shadow-blue-400/20' 
          : 'border-gray-600 hover:border-gray-500'
        }
      `}
      style={{ 
        backgroundColor: '#1a1a1a',
        boxShadow: selected 
          ? `0 0 20px ${config.color}40` 
          : '0 4px 12px rgba(0,0,0,0.3)'
      }}
    >
      {/* Node header */}
      <div className="flex items-center gap-2 mb-2">
        <div 
          className="w-6 h-6 rounded flex items-center justify-center text-xs"
          style={{ backgroundColor: `${config.color}20`, color: config.color }}
        >
          {config.icon}
        </div>
        <div className="flex-1">
          <div className="text-sm font-medium text-white truncate">
            {data.label}
          </div>
          <div className="text-xs text-gray-400">
            #{data.ptr}
          </div>
        </div>
      </div>

      {/* Status indicator */}
      <div className="flex items-center gap-1 text-xs text-gray-500">
        <div 
          className="w-2 h-2 rounded-full"
          style={{ backgroundColor: config.color }}
        />
        <span>Ready</span>
      </div>

      {/* Handles */}
      <Handle
        type="target"
        position={Position.Left}
        className="w-3 h-3 !bg-gray-600 !border-2 !border-gray-400 hover:!bg-gray-400"
        style={{ left: -6 }}
      />
      <Handle
        type="source"
        position={Position.Right}
        className="w-3 h-3 !bg-gray-600 !border-2 !border-gray-400 hover:!bg-gray-400"
        style={{ right: -6 }}
      />
    </div>
  );
});

CustomNode.displayName = 'CustomNode';