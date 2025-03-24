import { NextResponse } from "next/server";

export function middleware(request) {
  const { nextUrl } = request;

  // Allow access to the homepage and API routes
  if (nextUrl.pathname === "/") {
    return NextResponse.next();
  }

  // Redirect all direct refresh attempts to the landing page
  if (!request.headers.get("referer")) {
    return NextResponse.redirect(new URL("/", request.url));
  }

  return NextResponse.next();
}

export const config = {
  matcher: ["/((?!api|_next|favicon.ico).*)"], // Ignore _next, API, and assets
};
