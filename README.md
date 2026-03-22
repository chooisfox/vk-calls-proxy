# VK Calls Proxy

## How to build?

Well, the only thing you need is a `gcc toolchain`, `cmake`, and `qt6` with `QWebEngine` support.

Once you have everything installed, you can run:
```bash
cmake . -B ./build && cmake --build ./build
```

Also, you can try installing it with `--install`, but I have no fucking idea if it is working. Running the app from outside the build directory will probably fail.

## How do I use it?

You should have at least one VK account that is running on the server.

### Server setup

1.  First, you should switch the app to **`Creator Mode`** on your server and go to `https://vk.com/`, where you should log in.

2.  Then press the **`Start Service`** button and go to the `https://vk.com/calls` page.

3.  Once you are there, press the **`Create & Host Call`** button.
    *(Also, you can fill the user invite field. If you have a second account, it will allow it to autoconnect from the client).*

4.  After creating the call, the invite link should appear. You should copy it and insert it into the application that is in `Client Mode`.

### Client setup

1.  Press the **`Start Service`** button.

2.  Now, choose what you want to do:
    *   **If you have no account:** Just insert the invite link and press **`Join as Anon`**.
    *   **If you have an account:** Login as you did in server mode, then go to `https://vk.com/calls` and press **`Auto-Join`**.

Everything should work now.

### SOCKS5 setup

Just enter this in any local app that supports SOCKS5: `127.0.0.1:<port you selected>`

## P.S.

* Yeah, cmake scripts take more space than c++ code, I know. I like cmake, what are you going to do about it? 
* Can't compile on Ubunut, you know what? Go fuck yourself. You can either install real linux distro, or edit cmake scripts manually.
