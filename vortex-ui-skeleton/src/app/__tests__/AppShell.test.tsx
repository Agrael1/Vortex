import React from 'react';
import { describe, expect, it, vi } from 'vitest';
import { render } from '@testing-library/react';
import { RecoilRoot } from 'recoil';
import { AppShell } from '@/app/AppShell';

vi.mock('react-mosaic-component', () => {
  const MockContainer = ({ children }: { children?: React.ReactNode }) => <div data-testid="mosaic">{children}</div>;
  return {
    Mosaic: MockContainer,
    MosaicWindow: MockContainer,
  };
});

vi.mock('react-mosaic-component/react-mosaic-component.css', () => ({}));

describe('AppShell', () => {
  it('mounts router without crashing', () => {
    const { container } = render(
      <RecoilRoot>
        <AppShell />
      </RecoilRoot>,
    );
    expect(container.firstElementChild).toBeTruthy();
  });
});
