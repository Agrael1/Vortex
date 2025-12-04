import { memo } from 'react';
import { Handle, Position } from '@xyflow/react';

export interface CustomNodeData extends Record<string, unknown> {
  label: string;
  ptr: number;
  type?: string;
  props?: Record<string, unknown>;
  uid?: string | null;
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

const STATUS_VARIANTS: Record<string, { color: string; icon: string; label: string }> = {
  idle: { color: '#6b7280', icon: '●', label: 'Idle' },
  ready: { color: '#10b981', icon: '●', label: 'Ready' },
  busy: { color: '#f97316', icon: '●', label: 'Processing' },
  warning: { color: '#f59e0b', icon: '⚠️', label: 'Warning' },
  error: { color: '#ef4444', icon: '⚠️', label: 'Error' },
  offline: { color: '#94a3b8', icon: '⏻', label: 'Offline' },
  syncing: { color: '#38bdf8', icon: '↻', label: 'Syncing' },
  critical: { color: '#dc2626', icon: '⛔', label: 'Critical' },
};

const INDICATOR_ICONS: Record<string, { icon: string; label: string }> = {
  audio: { icon: '🔊', label: 'Audio' },
  video: { icon: '🎥', label: 'Video' },
  network: { icon: '🌐', label: 'Network' },
  record: { icon: '⏺', label: 'Recording' },
  ndi: { icon: '📡', label: 'NDI' },
  gpu: { icon: '🖥️', label: 'GPU' },
  cpu: { icon: '🧠', label: 'CPU' },
  storage: { icon: '💾', label: 'Storage' },
  memory: { icon: '📦', label: 'Memory' },
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

const collectIndicators = (props?: Record<string, unknown>) => {
  if (!props) return [];
  const set = new Set<string>();
  const merge = (value: unknown) => {
    if (typeof value === 'string') {
      set.add(value.toLowerCase());
    }
  };

  if (Array.isArray(props.indicators)) {
    props.indicators.forEach(merge);
  }

  if (props.audio === true || props.audioIn === true || props.audioOut === true) {
    set.add('audio');
  }
  if (props.video === true || props.videoIn === true || props.videoOut === true) {
    set.add('video');
  }
  if (props.network === true || typeof props.bitrate === 'number') {
    set.add('network');
  }
  if (props.record === true) {
    set.add('record');
  }
  if (props.ndi === true) {
    set.add('ndi');
  }
  if (props.gpu === true) {
    set.add('gpu');
  }
  if (props.cpu === true) {
    set.add('cpu');
  }
  if (props.storage === true || typeof props.diskUsage === 'number') {
    set.add('storage');
  }
  if (props.memory === true || typeof props.memoryUsage === 'number') {
    set.add('memory');
  }

  const resolved = Array.from(set)
    .map((key) => ({ key, entry: INDICATOR_ICONS[key] }))
    .filter((item) => Boolean(item.entry))
    .slice(0, 4);
  return resolved as { key: string; entry: { icon: string; label: string } }[];
};

const collectAlerts = (props?: Record<string, unknown>) => {
  if (!props) return [];
  const alerts: string[] = [];
  if (Array.isArray(props.alerts)) {
    props.alerts.forEach((item) => {
      if (typeof item === 'string' && item.trim().length) {
        alerts.push(item.trim());
      }
    });
  }
  if (typeof props.errorMessage === 'string') {
    alerts.push(props.errorMessage);
  }
  if (typeof props.warningMessage === 'string') {
    alerts.push(props.warningMessage);
  }
  return alerts.slice(0, 2);
};

const collectBadges = (props?: Record<string, unknown>) => {
  if (!props) return [] as string[];
  const badges: string[] = [];
  if (Array.isArray(props.tags)) {
    props.tags.forEach((tag) => {
      if (typeof tag === 'string' && tag.trim().length) {
        badges.push(tag.trim());
      }
    });
  }
  if (typeof props.profile === 'string') {
    badges.push(props.profile);
  }
  if (typeof props.scene === 'string') {
    badges.push(props.scene);
  }
  return badges.slice(0, 3);
};

type MetricEntry = { label: string; value: string };

const formatBitrate = (value: number) => {
  if (!Number.isFinite(value)) return null;
  if (value >= 1000) {
    return `${(value / 1000).toFixed(value >= 10000 ? 0 : 1)} Mbps`;
  }
  return `${value} kbps`;
};

const formatPercent = (value: number) => {
  if (!Number.isFinite(value)) return null;
  const clamped = Math.min(Math.max(value, 0), 100);
  return `${Math.round(clamped)}%`;
};

const collectMetrics = (props?: Record<string, unknown>): MetricEntry[] => {
  if (!props) return [];
  const entries: MetricEntry[] = [];

  const bitrate = formatBitrate(typeof props.bitrate === 'number' ? props.bitrate : Number.NaN);
  if (bitrate) entries.push({ label: 'Bitrate', value: bitrate });

  const latency = typeof props.latencyMs === 'number' ? props.latencyMs : typeof props.latency === 'number' ? props.latency : null;
  if (latency != null && Number.isFinite(latency)) {
    entries.push({ label: 'Latency', value: `${Math.round(latency)} ms` });
  }

  if (typeof props.droppedFrames === 'number' && Number.isFinite(props.droppedFrames)) {
    entries.push({ label: 'Dropped', value: `${props.droppedFrames}` });
  }

  const cpu = formatPercent(typeof props.cpuLoad === 'number' ? props.cpuLoad : Number.NaN);
  if (cpu) entries.push({ label: 'CPU', value: cpu });

  const gpu = formatPercent(typeof props.gpuLoad === 'number' ? props.gpuLoad : Number.NaN);
  if (gpu) entries.push({ label: 'GPU', value: gpu });

  if (typeof props.queueDepth === 'number' && Number.isFinite(props.queueDepth)) {
    entries.push({ label: 'Queue', value: `${props.queueDepth}` });
  }

  const uptimeSeconds =
    typeof props.uptimeSeconds === 'number'
      ? props.uptimeSeconds
      : typeof props.uptimeMs === 'number'
        ? props.uptimeMs / 1000
        : null;
  if (uptimeSeconds != null && Number.isFinite(uptimeSeconds) && uptimeSeconds > 0) {
    const hours = Math.floor(uptimeSeconds / 3600);
    const minutes = Math.floor((uptimeSeconds % 3600) / 60);
    const label = hours > 0 ? `${hours}h ${minutes}m` : `${minutes}m`;
    entries.push({ label: 'Uptime', value: label.trim() });
  }

  return entries.slice(0, 4);
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
    statusEntry.label;
  const detailText = typeof props.message === 'string' ? props.message : null;
  const progressValue = clampProgress(typeof props.progress === 'number' ? props.progress : NaN);
  const chips = pickMeta(props);
  const indicators = collectIndicators(props);
  const alerts = collectAlerts(props);
  const metrics = collectMetrics(props);
  const badges = collectBadges(props);
  const previewUrl =
    typeof props.preview === 'string'
      ? props.preview
      : typeof props.thumbnail === 'string'
        ? props.thumbnail
        : null;

  return (
    <div
      className={`
        relative px-4 py-3 rounded-2xl border transition-all duration-200
        min-w-[150px] max-w-[240px] bg-[#081121]
        ${selected ? 'border-transparent ring-2 ring-[#8FD3FF] shadow-lg shadow-[#8FD3FF]/30' : 'border-gray-700/60'}
      `}
      style={{
        background: 'linear-gradient(145deg, rgba(9,17,33,0.98), rgba(6,12,24,0.95))',
        boxShadow: selected ? `0 0 28px ${accentColor}40` : '0 12px 30px rgba(3, 7, 18, 0.55)',
        borderColor: selected ? accentColor : 'rgba(50,74,115,0.6)',
      }}
    >
      {previewUrl && (
        <div className="mb-2 rounded-lg overflow-hidden border border-white/5" data-testid="node-preview">
          <div className="relative">
            <img src={previewUrl} alt={`${data.label} preview`} className="h-20 w-full object-cover" draggable={false} />
            <div className="absolute inset-0 bg-gradient-to-b from-black/10 to-black/60" />
            <div className="absolute bottom-1 right-2 text-[10px] uppercase tracking-wide text-white/80">live feed</div>
          </div>
        </div>
      )}

      {/* Node header */}
      <div className="flex items-start gap-2 mb-2">
        <div
          className="w-8 h-8 rounded-lg flex items-center justify-center text-sm"
          style={{ backgroundColor: `${accentColor}25`, color: accentColor }}
        >
          {config.icon}
        </div>
        <div className="flex-1 min-w-0">
          <div className="text-sm font-semibold text-white truncate" title={data.label}>
            {data.label}
          </div>
          <div className="text-[11px] text-gray-400">#{data.ptr}</div>
        </div>
        <div className="flex flex-col items-end gap-1">
          <span
            className="text-[10px] px-2 py-0.5 rounded-full bg-white/5 text-gray-200"
            style={{ border: `1px solid ${statusEntry.color}40`, color: statusEntry.color }}
            data-testid="node-status"
          >
            <span className="mr-1">{statusEntry.icon}</span>
            {statusEntry.label}
          </span>
          {alerts.length > 0 && (
            <div className="flex flex-col gap-1 items-end" data-testid="node-alerts">
              {alerts.map((alert) => (
                <span key={alert} className="px-2 py-0.5 rounded-full bg-red-500/20 text-[10px] text-red-200">
                  {alert}
                </span>
              ))}
            </div>
          )}
        </div>
      </div>

      {indicators.length > 0 && (
        <div className="flex items-center gap-2 mb-2 text-gray-200" data-testid="node-indicators">
          {indicators.map(({ key, entry }) => (
            <span key={key} className="flex items-center gap-1 text-[11px] px-1.5 py-0.5 rounded bg-white/5">
              <span>{entry.icon}</span>
              <span>{entry.label}</span>
            </span>
          ))}
        </div>
      )}

      {badges.length > 0 && (
        <div className="mb-2 flex flex-wrap gap-1" data-testid="node-badges">
          {badges.map((badge) => (
            <span key={badge} className="rounded-full bg-white/5 px-2 py-0.5 text-[10px] text-gray-300">
              {badge}
            </span>
          ))}
        </div>
      )}

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

      {metrics.length > 0 && (
        <div className="mt-3 grid grid-cols-2 gap-2 text-[10px] text-gray-400" data-testid="node-metrics">
          {metrics.map((metric) => (
            <div key={`${metric.label}-${metric.value}`} className="flex justify-between gap-2">
              <span className="uppercase tracking-wide text-gray-500">{metric.label}</span>
              <span className="text-gray-200">{metric.value}</span>
            </div>
          ))}
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
