import { memo } from 'react';
import { Handle, Position } from '@xyflow/react';

export interface CustomNodeData extends Record<string, unknown> {
  label: string;
  ptr: number;
  type?: string;
  props?: Record<string, unknown>;
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

const STATUS_VARIANTS: Record<string, { color: string; icon: string }> = {
  idle: { color: '#6b7280', icon: '●' },
  ready: { color: '#10b981', icon: '●' },
  busy: { color: '#f97316', icon: '●' },
  warning: { color: '#f59e0b', icon: '⚠️' },
  error: { color: '#ef4444', icon: '⚠️' },
};

const clampProgress = (value: number) => {
  if (!Number.isFinite(value)) return null;
  if (value < 0) return 0;
  if (value > 100) return 100;
  return value;
};

const pickMeta = (props?: Record<string, unknown>) => {
  if (!props) return [];
  const chips: string[] = [];
  if (typeof props.fps === 'number') {
    chips.push(`${Math.round(props.fps)} fps`);
  }
  if (typeof props.resolution === 'string') {
    chips.push(props.resolution);
  } else if (typeof props.width === 'number' && typeof props.height === 'number') {
    chips.push(`${props.width}×${props.height}`);
  }
  if (typeof props.format === 'string') {
    chips.push(props.format);
  }
  if (typeof props.mode === 'string') {
    chips.push(props.mode);
  }
  return chips.slice(0, 3);
};

export const CustomNode = memo(({ data, selected }: CustomNodeProps) => {
  const props = (data.props as Record<string, unknown>) ?? {};
  const nodeType = data.type || data.label;
  const config = NODE_TYPES[nodeType as keyof typeof NODE_TYPES] || {
    color: '#6b7280',
    icon: '⚙️',
  };

  const hasError = props.error === true;
  const hasWarning = props.warning === true;
  const isBusy = props.busy === true;
  const severity = (typeof props.severity === 'string' && props.severity.toLowerCase()) ||
    (hasError ? 'error' : hasWarning ? 'warning' : isBusy ? 'busy' : 'ready');
  const statusEntry = STATUS_VARIANTS[severity] ?? STATUS_VARIANTS.ready;
  const accentColor = typeof props.accent === 'string' ? props.accent : statusEntry.color ?? config.color;
  const statusText =
    (typeof props.status === 'string' && props.status) ||
    (typeof props.state === 'string' && props.state) ||
    (isBusy ? 'Working…' : hasError ? 'Error' : 'Ready');
  const detailText = typeof props.message === 'string' ? props.message : null;
  const progressValue = clampProgress(typeof props.progress === 'number' ? props.progress : NaN);
  const chips = pickMeta(props);

  return (
    <div
      className={`
        relative px-4 py-3 rounded-lg border-2 transition-all duration-200
        min-w-[140px] max-w-[220px]
        ${selected ? 'border-blue-400 shadow-lg shadow-blue-400/20' : 'border-gray-600 hover:border-gray-500'}
      `}
      style={{
        backgroundColor: '#111315',
        boxShadow: selected ? `0 0 20px ${accentColor}40` : '0 4px 12px rgba(0,0,0,0.3)',
      }}
    >
      {/* Node header */}
      <div className="flex items-center gap-2 mb-2">
        <div
          className="w-7 h-7 rounded flex items-center justify-center text-xs"
          style={{ backgroundColor: `${accentColor}20`, color: accentColor }}
        >
          {config.icon}
        </div>
        <div className="flex-1 min-w-0">
          <div className="text-sm font-medium text-white truncate" title={data.label}>
            {data.label}
          </div>
          <div className="text-[11px] text-gray-400">#{data.ptr}</div>
        </div>
      </div>

      {/* Status indicator */}
      <div className="flex items-center gap-2 text-xs text-gray-300">
        <div className="flex items-center gap-1 text-[11px] uppercase tracking-wide">
          <span className="text-sm" style={{ color: statusEntry.color }}>
            {statusEntry.icon}
          </span>
          <span className="text-[11px] text-gray-300">{statusText}</span>
        </div>
        {chips.length > 0 && (
          <div className="flex flex-wrap gap-1 ml-auto">
            {chips.map((chip) => (
              <span key={chip} className="px-1.5 py-0.5 rounded bg-white/5 text-[10px] text-gray-300">
                {chip}
              </span>
            ))}
          </div>
        )}
      </div>

      {detailText && <div className="mt-1 text-[11px] text-gray-400 line-clamp-2">{detailText}</div>}

      {progressValue != null && (
        <div className="mt-2 h-1.5 rounded bg-white/10">
          <div className="h-full rounded" style={{ width: `${progressValue}%`, backgroundColor: accentColor }} />
        </div>
      )}

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
