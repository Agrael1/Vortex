import React from 'react';
import { beforeEach, describe, expect, it, vi } from 'vitest';
import '@testing-library/jest-dom/vitest';
import { fireEvent, render, screen, waitFor } from '@testing-library/react';
import { Hub } from '@/app/routes/Hub';
import { RECENTS_STORAGE_KEY } from '@/app/routes/helpers/recents';

const mockNavigate = vi.hoisted(() => vi.fn());
const mockLoadProject = vi.hoisted(() => vi.fn());
const mockPushNotification = vi.hoisted(() => vi.fn());
const mockLocation = vi.hoisted(() => ({ pathname: '/hub', state: null as unknown })) as { pathname: string; state: unknown };
const engineListeners = vi.hoisted(() => new Map<string, Set<(payload: unknown) => void>>()) as Map<
  string,
  Set<(payload: unknown) => void>
>;
const mockEngine = vi.hoisted(() => ({
  listRecent: vi.fn(),
  browseForProject: vi.fn(),
  browseForFolder: vi.fn(),
  createProject: vi.fn(),
  getLastProject: vi.fn(),
  clearLastProject: vi.fn(),
  on: vi.fn<(event: string, handler: (payload: unknown) => void) => () => void>(),
})) as {
  listRecent: ReturnType<typeof vi.fn>;
  browseForProject: ReturnType<typeof vi.fn>;
  browseForFolder: ReturnType<typeof vi.fn>;
  createProject: ReturnType<typeof vi.fn>;
  getLastProject: ReturnType<typeof vi.fn>;
  clearLastProject: ReturnType<typeof vi.fn>;
  on: ReturnType<typeof vi.fn>;
};

vi.mock('react-router-dom', () => ({
  useNavigate: () => mockNavigate,
  useLocation: () => mockLocation,
}));

vi.mock('@state/hooks/useProjectCommands', () => ({
  useProjectCommands: () => ({ loadProject: mockLoadProject }),
}));

vi.mock('@state/hooks/useNotificationCenter', () => ({
  useNotificationCenter: () => ({ push: mockPushNotification, dismiss: vi.fn(), clearAll: vi.fn() }),
}));

vi.mock('@/app/components/CreateProjectModal', () => ({
  CreateProjectModal: ({ isOpen }: { isOpen: boolean }) => (isOpen ? <div data-testid="create-project-modal" /> : null),
}));


vi.mock('@/app/services/ipc/cefBridge', () => ({
  engine: mockEngine,
}));

const baseLastProject = {
  name: 'Orion Broadcast',
  path: 'C:/projects/orion.vortex',
  lastOpened: '2025-11-20T10:00:00.000Z',
  template: 'Blank Project',
};

const sampleRecent = {
  name: 'Example',
  path: 'C:/demo/example.vortex',
  last: '2025-11-18T09:00:00.000Z',
};

const resolveListeners = () => {
  mockEngine.on.mockImplementation((event: string, handler: (payload: unknown) => void) => {
    const bucket = engineListeners.get(event) ?? new Set();
    bucket.add(handler);
    engineListeners.set(event, bucket);
    return () => {
      bucket.delete(handler);
      if (bucket.size === 0) {
        engineListeners.delete(event);
      }
    };
  });
};

describe('Hub route', () => {
  beforeEach(() => {
    localStorage.clear();
    mockNavigate.mockReset();
    mockLoadProject.mockReset();
    mockPushNotification.mockReset();

    mockEngine.listRecent.mockReset();
    mockEngine.listRecent.mockResolvedValue([sampleRecent]);
    mockEngine.browseForProject.mockReset();
    mockEngine.browseForProject.mockResolvedValue(null);
    mockEngine.browseForFolder.mockReset();
    mockEngine.browseForFolder.mockResolvedValue(null);
    mockEngine.createProject.mockReset();
    mockEngine.createProject.mockResolvedValue({
      name: 'Example',
      path: 'C:/demo/example.vortex',
      template: null,
      settings: { width: 1920, height: 1080, fps: 60, colorSpace: 'Rec.709' },
    });
    mockEngine.getLastProject.mockReset();
    mockEngine.getLastProject.mockReturnValue({ ...baseLastProject });
    mockEngine.clearLastProject.mockReset();
    mockEngine.clearLastProject.mockImplementation(() => undefined);
    engineListeners.clear();
    mockEngine.on.mockReset();
    resolveListeners();

    mockLoadProject.mockResolvedValue(undefined);
  });

  const renderHub = () => render(<Hub />);

  const waitForInitialLoad = async () => {
    await waitFor(() => expect(mockEngine.listRecent).toHaveBeenCalled());
  };

  it('continues the last project and navigates to the editor', async () => {
    renderHub();
    await waitForInitialLoad();

    const continueButton = await screen.findByRole('button', { name: 'Continue' });
    fireEvent.click(continueButton);

    await waitFor(() => expect(mockLoadProject).toHaveBeenCalledWith(baseLastProject.path));
    await waitFor(() => expect(mockNavigate).toHaveBeenCalledWith('/editor'));
  });

  it('forgets the last project and hides CTA', async () => {
    renderHub();
    await waitForInitialLoad();

    const forgetButton = await screen.findByRole('button', { name: 'Forget this project' });
    fireEvent.click(forgetButton);

    expect(mockEngine.clearLastProject).toHaveBeenCalledTimes(1);
    await waitFor(() => expect(screen.queryByText('Continue last project')).toBeNull());
  });

  it('persists auto-continue preferences to storage', async () => {
    renderHub();
    await waitForInitialLoad();

    const autoCheckbox = await screen.findByLabelText('Auto-continue on launch');
    fireEvent.click(autoCheckbox);
    await waitFor(() => expect(window.localStorage.getItem('vortex.splash.autoContinue')).toBe('true'));

    const delaySelect = await screen.findByLabelText('Auto-continue delay');
    fireEvent.change(delaySelect, { target: { value: '3000' } });
    await waitFor(() => expect(window.localStorage.getItem('vortex.splash.autoDelay')).toBe('3000'));

    const fallbackSelect = await screen.findByLabelText(/Splash fallback delay/i);
    fireEvent.change(fallbackSelect, { target: { value: '5000' } });
    await waitFor(() => expect(window.localStorage.getItem('vortex.splash.fallbackDelay')).toBe('5000'));
  });

  it('opens a recent project via keyboard navigation and refreshes the CTA', async () => {
    renderHub();
    await waitForInitialLoad();

    const listbox = await screen.findByRole('listbox', { name: 'Recent projects' });
    listbox.focus();
    fireEvent.keyDown(listbox, { key: 'ArrowDown' });

    fireEvent.keyDown(listbox, { key: 'Enter' });

    await waitFor(() => expect(mockLoadProject).toHaveBeenCalledWith(sampleRecent.path));
    await waitFor(() => expect(mockNavigate).toHaveBeenCalledWith('/editor'));
    await waitFor(() => expect(screen.getByText(/Back to example/i)).toBeInTheDocument());
  });

  it('supports pinning and removing recents with shortcuts', async () => {
    renderHub();
    await waitForInitialLoad();

    const listbox = await screen.findByRole('listbox', { name: 'Recent projects' });
    listbox.focus();
    fireEvent.keyDown(listbox, { key: 'ArrowDown' });
    fireEvent.keyDown(listbox, { key: 'p', ctrlKey: true });

    await waitFor(() => expect(screen.getByText('★')).toBeInTheDocument());

    fireEvent.keyDown(listbox, { key: 'Delete' });

    await waitFor(() => expect(screen.getByText('Your projects will appear here')).toBeInTheDocument());
    const stored = window.localStorage.getItem(RECENTS_STORAGE_KEY);
    expect(stored).toBe('[]');
  });

  it('renames a recent entry and persists the new label', async () => {
    renderHub();
    await waitForInitialLoad();

    const overflowButton = await screen.findByRole('button', { name: '⋮' });
    fireEvent.click(overflowButton);

    const renameMenu = await screen.findByRole('button', { name: 'Rename…' });
    fireEvent.click(renameMenu);

    const input = await screen.findByPlaceholderText('Project name');
    fireEvent.change(input, { target: { value: 'Stage Lighting' } });

    const saveButton = screen.getByRole('button', { name: 'Save' });
    fireEvent.click(saveButton);

    await waitFor(() => expect(screen.getByText('Stage Lighting')).toBeInTheDocument());

    const stored = JSON.parse(window.localStorage.getItem(RECENTS_STORAGE_KEY) ?? '[]');
    expect(stored[0]?.name).toBe('Stage Lighting');
  });

  it('pins and unpins the last project from the CTA banner', async () => {
    renderHub();
    await waitForInitialLoad();

    const pinButton = await screen.findByRole('button', { name: 'Pin in recents' });
    fireEvent.click(pinButton);

    await waitFor(() => {
      const stored = JSON.parse(window.localStorage.getItem(RECENTS_STORAGE_KEY) ?? '[]');
      const entry = stored.find((item: any) => item.path === baseLastProject.path);
      expect(entry?.pinned).toBe(true);
    });

    const unpinButton = await screen.findByRole('button', { name: 'Unpin from recents' });
    fireEvent.click(unpinButton);

    await waitFor(() => {
      const stored = JSON.parse(window.localStorage.getItem(RECENTS_STORAGE_KEY) ?? '[]');
      const entry = stored.find((item: any) => item.path === baseLastProject.path);
      expect(entry?.pinned).toBeUndefined();
    });
  });

  it('highlights search matches within recent entries', async () => {
    renderHub();
    await waitForInitialLoad();

    const searchField = screen.getByPlaceholderText(/Search projects/);
    fireEvent.change(searchField, { target: { value: 'exa' } });

    const highlights = await screen.findAllByTestId('search-highlight');
    expect(highlights.length).toBeGreaterThan(0);
    expect(highlights[0]).toHaveTextContent(/exa/i);
  });

  it('pushes notification when project open fails', async () => {
    mockLoadProject.mockRejectedValueOnce(new Error('Disk missing'));

    renderHub();
    await waitForInitialLoad();

    const continueButton = await screen.findByRole('button', { name: 'Continue' });
    fireEvent.click(continueButton);

    await waitFor(() => expect(mockPushNotification).toHaveBeenCalled());
    const lastCall = mockPushNotification.mock.calls[mockPushNotification.mock.calls.length - 1];
    const payload = lastCall?.[0];
    expect(payload).toMatchObject({ title: 'Open project failed', message: 'Disk missing', level: 'error' });
  });

  it('executes inline pin and rename actions from recents card', async () => {
    renderHub();
    await waitForInitialLoad();

    const inlinePin = await screen.findByRole('button', { name: 'Pin project' });
    fireEvent.click(inlinePin);

    await waitFor(() => {
      const stored = window.localStorage.getItem(RECENTS_STORAGE_KEY);
      expect(stored).toContain('"pinned":true');
    });

    const inlineRename = await screen.findByRole('button', { name: 'Rename project' });
    fireEvent.click(inlineRename);

    const input = await screen.findByPlaceholderText('Project name');
    fireEvent.change(input, { target: { value: 'Inline Edited' } });
    fireEvent.click(screen.getByRole('button', { name: 'Save' }));

    await waitFor(() => expect(screen.getByText('Inline Edited')).toBeInTheDocument());
  });

  it('renders error badge when project entry has an error flag', async () => {
    mockEngine.listRecent.mockResolvedValueOnce([
      { name: 'Broken Scene', path: 'C:/broken.vortex', last: '2025-11-20T10:00:00.000Z', error: 'Checksum failed' },
    ]);

    renderHub();
    await waitForInitialLoad();

    const badge = await screen.findByText(/Issue|Checksum failed/i);
    expect(badge).toBeInTheDocument();
    expect(badge).toHaveAttribute('title', 'Checksum failed');
  });
});
