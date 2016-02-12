# farbfeld-resize
farbfeld image resizing filter

This filter uses catmull-rom interpolation to resize images.

As befits the minimalist approach of the farbfeld image format, this image
resizer focuses on clean and concise code while staying as accurate as possible.

It has no dependencies.

## Usage

Resize an image to fit into a 1300x800 box while maintaining the aspect ratio:

```
png2ff < in.png | ./resize 1300 800 | ff2png > out.png
```

## Technical Details

This resizer uses a single interpolation function, `xscale()`. Which only knows
how to perform horizontal scaling.

The trick to vertical scaling is that we make two passes on the image,
transposing pixels during each pass.

For example, when we resize a 500x600 image to 250x300, the first
resize-transpose pass will return a 600x250 image.

The second resize-transpose pass returns a 250x300 image and results in
horizontal scaling.

## FAQ

### I want to resize my image to be 400px wide and I don't care about the height. How do I do this?

Are you sure that there isn't a height limit? If you think about it, you might
realize that there is in fact a limit to how tall you want your output image
to be.

Imagine a very narrow & tall image comes along, are you sure you want it to
grow to 100,000 pixels?

I am curious about such use cases. Please reach out to me if you have feedback
on this.

### There is an image shift. Why?

There is a 1/2 pixel image shift due to mapping pixel edges instead of pixel
centers.

This is handled in [liboil](https://github.com/ender672/liboil) but as an
experiment on whether this is really needed I left it out.

I've yet to see a real-world case where this matters. If you have one, I would
like to hear from you.

### You are clamping intermediate values. Why?

I have yet to see a real-world case where this is visible. If you have one, I
would like to hear from you.

### How well does it handle transparency?

The alpha channel is treated just like other channels, meaning the resulting
image can end up with transparency artifacts.
