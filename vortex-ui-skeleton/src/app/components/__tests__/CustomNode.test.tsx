import React from 'react';
import { describe, expect, it, vi } from 'vitest';
import { render, screen } from '@testing-library/react';
import { CustomNode, type CustomNodeData } from '@/app/components/CustomNode';

vi.mock('@xyflow/react', () => ({
  Handle: ({ type }: { type: string }) => <div data-testid={`handle-${type}`} />,
  Position: { Left: 'left', Right: 'right' },
}));

describe('CustomNode', () => {
  const baseData: CustomNodeData = {
    label: 'Stream Input',
    ptr: 99,
    type: 'StreamInput',
    props: {},
  };

  const renderNode = (override?: Partial<CustomNodeData>) => {
    return render(<CustomNode data={{ ...baseData, ...override }} />);
  };

  it('renders preview image and alerts when provided', () => {
    renderNode({
      props: {
        preview: 'https://example.com/frame.jpg',
        alerts: ['Packet loss'],
      },
    });

    expect(screen.getByTestId('node-preview')).toBeInTheDocument();
    expect(screen.getByTestId('node-alerts')).toHaveTextContent('Packet loss');
  });

  it('shows indicator icons derived from props', () => {
    renderNode({
      props: {
        audio: true,
        video: true,
        indicators: ['network'],
      },
    });

    const indicators = screen.getByTestId('node-indicators');
    expect(indicators.textContent).toContain('Audio');
    expect(indicators.textContent).toContain('Video');
    expect(indicators.textContent).toContain('Network');
  });

  it('renders status badge and metrics when props are provided', () => {
    renderNode({
      props: {
        severity: 'offline',
        bitrate: 2500,
        latencyMs: 42,
        cpuLoad: 73,
        uptimeSeconds: 5400,
      },
    });

    expect(screen.getByTestId('node-status')).toHaveTextContent('Offline');

    const metrics = screen.getByTestId('node-metrics');
    expect(metrics.textContent).toContain('Bitrate');
    expect(metrics.textContent).toContain('2.5 Mbps');
    expect(metrics.textContent).toContain('Latency');
    expect(metrics.textContent).toContain('42 ms');
    expect(metrics.textContent).toContain('CPU');
    expect(metrics.textContent).toContain('73%');
    expect(metrics.textContent).toContain('Uptime');
    expect(metrics.textContent).toContain('1h 30m');
  });

  it('renders badges and extended indicators', () => {
    renderNode({
      props: {
        tags: ['Primary', 'Stage'],
        scene: 'Studio',
        diskUsage: 0.7,
        memoryUsage: 0.4,
      },
    });

    const badges = screen.getByTestId('node-badges');
    expect(badges.textContent).toContain('Primary');
    expect(badges.textContent).toContain('Studio');

    const indicators = screen.getByTestId('node-indicators');
    expect(indicators.textContent).toContain('Storage');
    expect(indicators.textContent).toContain('Memory');
  });
});
