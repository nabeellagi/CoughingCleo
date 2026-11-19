import { createSignal, For } from "solid-js";

// Fake Reading Test!

const READINGS = [
    { label: "TVOC", value: "500", unit: "ppm", status: "Acceptable" },
    { label: "CO₂", value: "800", unit: "ppm", status: "Good" },
    { label: "PM2.5", value: "12", unit: "μg/m³", status: "Good" }
]

const maskTo = (url) => ({
    "-webkit-mask-image": `url("${url}")`,
    "mask-image": `url("${url}")`,
    "-webkit-mask-size": "100% 100%",
    "mask-size": "100% 100%",
    "-webkit-mask-repeat": "no-repeat",
    "mask-repeat": "no-repeat",
});

const CAT_CARD_SVG =
    "data:image/svg+xml," +
    encodeURIComponent(
        `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 365 237" preserveAspectRatio="none">
            <path d="M0 40 L12 40 L45 0 L88 40 L277 40 L306 0 L345 40 L365 40 L365 213 Q365 237 341 237 L24 237 Q0 237 0 213 Z"/>
        </svg>`
    );

const FabricOverlay = (props) => {
    return (
        <img
            src="/images/FabricTexture.png"
            className={`absolute inset-0 w-full h-full object-cover mix-blend-multiply pointer-events-none ${props.class ?? ""}`}
            alt=""
        />
    );
}

const MenuButton = (props) => {
    return (
        <button
            type="button"
            onClick={props.onClick}
            className={`relative overflow-hidden rounded-2xl bg-[#4a3535] min-h-24
                        shadow-[3px_6px_8px_rgba(30,70,80,0.45)]
                        active:scale-95 transition-transform
                        flex items-center justify-center px-6 ${props.class ?? ""}`}
        >
            <FabricOverlay class="opacity-30" />
            <span className="relative z-10 font-fingerpaint text-d-18 text-center text-[#eae1b6] leading-tight">
                {props.children}
            </span>
        </button>
    );
}


/* --------------------- Slide CONTENT ----------------- */

const ReadingSlide = (props) => {
    return (
        <div class="w-full shrink-0 snap-center snap-always px-6 pt-10 pb-8 flex flex-col">
            <h2 class="font-dynapuff text-[#eae1b6] text-d-20 self-end">
                {props.reading.label}
            </h2>

            <div class="mt-auto">
                <p class="font-dynapuff text-[#eae1b6] leading-none">
                    <span class="text-d-48">{props.reading.value}</span>
                    <span class="text-d-24 ml-1">{props.reading.unit}</span>
                </p>
                <p class="font-fingerpaint text-d-14 text-[#eae1b6] mt-2">
                    {props.reading.status}
                </p>
            </div>
        </div>
    )
}

/* -------------------------- MAIN PAGE --------------------------------- */

export default function Home() {
    const [active, setActive] = createSignal(0);
    const [dragging, setDragging] = createSignal(false);

    let track; // Ref
    let drag = null;

    const goTo = (i) => {
        const idx = Math.max(0, Math.min(READINGS.length - 1, i));
        track.scrollTo({ left : idx * track.clientWidth, behaviour: "smooth" }) 
    }

    const onScroll = () => {
        setActive(Math.round(track.scrollLeft / track.clientWidth));
    }

    const onPointerDown = (e) => {
        if(e.pointerType !== "mouse" || e.button !== 0) return;
        drag = { x: e.clientX, scroll: track.scrollLeft };
        track.setPointerCapture(e.pointerId);
        setDragging(true);
    }

    const onPointerMove = (e) => {
        if(!drag) return;
        track.scrollLeft = drag.scroll - (e.clientX - drag.x);
    }

    const endDrag = (e) => {
        if(!drag) return;
        const w = track.clientWidth;
        const dx = drag.x - e.clientX;
        let idx = Math.round(drag.scroll / w);
        if (Math.abs(dx) > w * 0.15) idx += dx > 0 ? 1 : -1;
        drag = null;
        setDragging(false);
        goTo(idx);
    }

    return (
        <div class="slim-scroll h-dvh w-full rounded-xl bg-[#3d88b2FF] overflow-y-auto overflow-x-hidden motion-safe:scroll-smooth">
            <div class="relative min-h-full w-full overflow-hidden">
                <FabricOverlay/>
                <div
                    class="absolute inset-x-0 bottom-0 top-[17%] isolate pointer-events-none origin-top"
                    style={{ transform: "scale(1.15)"}}
                >
                    <img
                        src = "/images/CatPaper.png"
                        class = "absolute inset-0 w-full h-full object-fill"
                    />
                    <div
                        class = "absolute inset-0 bg-[#eae1b6FF] mix-blend-multiply"
                        style={maskTo("/images/CatPaper.png")}
                    />
                </div>

                <div
                    class="relative z-10 px-6 pt-11.75 pb-10"
                >
                    <h1 class="font-dynapuff text-d-24 text-[#eae1b6FF]">Hello!</h1>
                    <h3 class="font-fingerpaint text-d-14 text-[#eae1b6FF] pt-2.5">
                        Welcome to Coughing Cleo's Menu!
                    </h3>

                    {/* Carousel */}
                    <section class="relative w-full mt-20 aspect-[365/237]">
                        <div
                            class="absolute inset-0 bg-[#4a3535] overflow-hidden"
                            style={maskTo(CAT_CARD_SVG)}
                        >
                            <FabricOverlay class="opacity-30"/>
                        </div>

                        <div
                            ref={track}
                            tabIndex="0"
                            aria-label="Air quality readings"
                            class="absolute inset-x-0 bottom-0 top-[17%] z-10 flex
                            overflow-x-auto overflow-y-hidden select-none
                            scrollbar-none [&::-webkit-scrollbar]:hidden"
                            classList = {{
                                "cursor-grab" : !dragging(),
                                "cursor-grabbing" : dragging()
                            }}
                            style={{
                                "scroll-snap-type" : dragging() ? "none" : "x mandatory"
                            }}
                            onScroll={onScroll}
                            onPointerDown={onPointerDown}
                            onPointerMove={onPointerMove}
                            onPointerUp={endDrag}
                            onPointerCancel={endDrag}
                        >
                            <For each={READINGS}>
                                {(reading) => <ReadingSlide reading={reading}/>}
                            </For>
                        </div>
                        <div
                            class="absolute bottom-3 left-1/2 -translate-x-1/2 flex gap-2 z-20"
                        >
                            <For each={READINGS}>
                                {(r, i) => (
                                    <button
                                        type="button"
                                        aria-label = {`Show ${r.label}`}
                                        onClick = {() => goTo(i())}
                                        class="w-2 h-2 rounded-full transition-colors"
                                        classList = {{
                                            "bg-[#f5c9a8]" : active() === i(),
                                            "bg-[#eae1b6FF]/40" : active() !== i()
                                        }}
                                    />
                                )}
                            </For>
                        </div>
                    </section>
                    <nav class="grid grid-cols-2 grid-rows-3 gap-3 mt-4 px-4">
                        <MenuButton class="row-span-2">Change Audio</MenuButton>
                        <MenuButton>Read Logs</MenuButton>
                        <MenuButton>Git Repo</MenuButton>
                        <MenuButton class="col-span-2">How to Use</MenuButton>
                    </nav>
                </div>
            </div>
        </div>
    )
}