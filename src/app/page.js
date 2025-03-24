"use client";
import { useRouter } from "next/navigation";
import { useEffect, useState } from "react";
const BackgroundVideo = () => {
  return (
    <video autoPlay loop muted className="absolute top-0 left-0 w-full h-full object-cover -z-10">
      <source src="/bg1.webm" type="video/mp4" />
      Your browser does not support the video tag.
    </video>
  );
};
export default function Page() {
  const router = useRouter();
  const [blink, setBlink] = useState(true);

  useEffect(() => {
    const interval = setInterval(() => {
      setBlink((prev) => !prev);
    }, 500);
    return () => clearInterval(interval);
  }, []);

  return (
    <div className="relative w-full h-screen flex items-center justify-center text-center">
      <BackgroundVideo />
      {/* Overlay */}
      <div className="absolute inset-0 bg-opacity-50"></div>

      {/* Content */}
      <div className="relative z-10 text-white">
        <h1 className="text-4xl md:text-6xl font-bold mb-8">
          Welcome to <span className="text-blue-500">RiscOrbit</span>
        </h1>
        <p className="text-lg md:text-2xl mb-12">
          Launch your ideas and codes into the{" "}
          <span className="text-green-500">RISC-V</span> universe.
        </p>
        <button
          onClick={() => router.push("/editor")}
          className={`px-6 py-3 text-lg font-semibold rounded-md transition-all ${
            blink
              ? "bg-blue-600 text-white"
              : "bg-transparent border-2 border-blue-600 text-blue-600"
          }`}
        >
          Go to Editor
        </button>
      </div>
    </div>
  );
}
