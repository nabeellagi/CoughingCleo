import { createSignal } from 'solid-js'
import Home from './components/Home'

function App() {
  return (
    <div class='h-dvh w-full bg-yellow-200 slim-scroll'>
      <div
        aria-hidden="true"
        class = "absolute inset-0 bg-[url('images/back_art.png')] bg-cover bg-center bg-no-repeat [image-rendering:pixelated]"
      />
      <main
        class="frame relative mx-auto h-dvh overflow-y-auto overflow-x-hidden shadow-amber-900 shadow-2xl"
        style={{ width: "min(100vw, calc(100dvh * 9 / 15.5))"}}
      >
        <Home/>
      </main>
    </div>
  )
}

export default App
