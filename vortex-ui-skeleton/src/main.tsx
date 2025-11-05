import React from 'react'
import ReactDOM from 'react-dom/client'
import { createBrowserRouter, RouterProvider } from 'react-router-dom'
import { RecoilRoot } from 'recoil'
import { Splash } from './app/routes/Splash'
import { Hub } from './app/routes/Hub'
import { Editor } from './app/routes/Editor'
import './app/theme/tailwind.css'

const router = createBrowserRouter([
  { path: '/', element: <Splash/> },
  { path: '/hub', element: <Hub/> },
  { path: '/editor', element: <Editor/> },
])

ReactDOM.createRoot(document.getElementById('root')!).render(
  <React.StrictMode>
    <RecoilRoot>
      <RouterProvider router={router} />
    </RecoilRoot>
  </React.StrictMode>,
)