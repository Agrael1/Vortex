import React from 'react';
import { describe, expect, it } from 'vitest';
import { render } from '@testing-library/react';
import { AppShell } from '@/app/AppShell';

describe('AppShell', () => {
  it('mounts router without crashing', () => {
    const { container } = render(<AppShell />);
    expect(container.firstElementChild).toBeTruthy();
  });
});
