import React from 'react';
import { MemoryRouter } from 'react-router-dom';
import { describe, it, beforeEach, expect, vi } from 'vitest';
import '@testing-library/jest-dom/vitest';
import { fireEvent, render, screen } from '@testing-library/react';
import { Splash } from '@/app/routes/Splash';

const mockLoadProject = vi.hoisted(() => vi.fn());
const mockEngine = vi.hoisted(() => ({
  getLastProject: vi.fn(),
  clearLastProject: vi.fn(),
}));

vi.mock('@state/hooks/useProjectCommands', () => ({
  useProjectCommands: () => ({ loadProject: mockLoadProject }),
}));

vi.mock('@/app/services/ipc/cefBridge', () => ({
  engine: mockEngine,
}));

describe('Splash route timing controls', () => {
  beforeEach(() => {
    window.localStorage.clear();
    mockLoadProject.mockReset();
    mockEngine.getLastProject.mockReset();
    mockEngine.getLastProject.mockReturnValue(null);
    mockEngine.clearLastProject.mockReset();
  });

  const renderSplash = () =>
    render(
      <MemoryRouter>
        <Splash />
      </MemoryRouter>,
    );

  it('persists timing preference changes when selectors update', async () => {
    renderSplash();

    const toggle = await screen.findByRole('button', { name: /Timing preferences/i });
    fireEvent.click(toggle);

    const autoSelect = await screen.findByLabelText(/Auto-continue delay/i);
    fireEvent.change(autoSelect, { target: { value: '2000' } });

    const fallbackSelect = await screen.findByLabelText(/Splash fallback delay/i);
    fireEvent.change(fallbackSelect, { target: { value: '5000' } });

    expect(window.localStorage.getItem('vortex.splash.autoDelay')).toBe('2000');
    expect(window.localStorage.getItem('vortex.splash.fallbackDelay')).toBe('5000');

    await screen.findByText('Auto 2s / Hub 5s');
  });

  it('shows formatted summary for custom delay presets', async () => {
    window.localStorage.setItem('vortex.splash.autoDelay', '4444');
    window.localStorage.setItem('vortex.splash.fallbackDelay', '9000');

    renderSplash();

    await screen.findByText('Auto 4.4s / Hub 9s');

    const toggle = await screen.findByRole('button', { name: /Timing preferences/i });
    expect(toggle).toHaveAttribute('aria-expanded', 'false');
    fireEvent.click(toggle);
    expect(toggle).toHaveAttribute('aria-expanded', 'true');
  });
});
