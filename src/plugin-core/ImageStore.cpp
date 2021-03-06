/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#include "ImageStore.h"
#include <QImage>
#include <QFile>

Image::Image(const QString &path, ImageStore &owner, int handle):
		owner(&owner),
		own_handle(handle),
		alphaed(false){
	if (!QFile::exists(path))
		throw ImageOperationResult("File not found.");
	this->bitmap = QImage(path);
	if (this->bitmap.isNull())
		throw ImageOperationResult("Unknown error.");
	this->w = this->bitmap.width();
	this->h = this->bitmap.height();
}

Image::Image(int w, int h, ImageStore &owner, int handle):
		owner(&owner),
		own_handle(handle),
		alphaed(false){
	this->bitmap = QImage(w, h, QImage::Format_RGBA8888);
	if (this->bitmap.isNull())
		throw ImageOperationResult("Unknown error.");
	this->w = this->bitmap.width();
	this->h = this->bitmap.height();
}

Image::Image(const QImage &image, ImageStore &owner, int handle):
		owner(&owner),
		own_handle(handle),
		alphaed(false){
	this->bitmap = image;
	if (this->bitmap.isNull())
		throw ImageOperationResult("Unknown error.");
	this->w = this->bitmap.width();
	this->h = this->bitmap.height();
}

void Image::traverse(traversal_callback cb){
	this->to_alpha();

	auto pixels = this->bitmap.bits();
	for (int y = 0; y < this->h; y++){
		//auto scanline = pixels + this->pitch * (this->h - 1 - y);
		auto scanline = pixels + this->pitch * y;
		for (int x = 0; x < this->w; x++){
			auto pixel = scanline + x * this->stride;
			int r = pixel[0];
			int g = pixel[1];
			int b = pixel[2];
			int a = pixel[3];
			auto prev = this->owner->get_current_traversal_image();
			auto prev_pixel = this->current_pixel;
			this->owner->set_current_traversal_image(this);
			this->current_pixel = pixel;
			cb(r, g, b, a, x, y);
			this->owner->set_current_traversal_image(prev);
			this->current_pixel = prev_pixel;
		}
	}
}

void Image::to_alpha(){
	if (this->alphaed)
		return;
	if (this->bitmap.format() != QImage::Format_RGBA8888)
		this->bitmap = this->bitmap.convertToFormat(QImage::Format_RGBA8888);
	this->pitch = this->bitmap.bytesPerLine();
	this->alphaed = true;
}

void Image::set_current_pixel(const pixel_t &rgba){
	for (int i = 0; i < 4; i++)
		this->current_pixel[i] = rgba[i];
}

ImageOperationResult Image::save(const QString &path, SaveOptions opt){
	ImageOperationResult ret;
	ret.success = this->bitmap.save(path, opt.format.size() ? opt.format.c_str() : nullptr, opt.compression);
	if (!ret.success)
		ret.message = "Unknown error.";
	return ret;
}

ImageOperationResult Image::get_pixel(unsigned x, unsigned y){
	if (x >= (unsigned)this->w || y >= (unsigned)this->h)
		return "Invalid coordinates.";
	this->to_alpha();
	auto pixels = this->bitmap.bits();
	auto pixel = pixels + this->pitch * y + this->stride * x;
	ImageOperationResult ret;
	ret.success = 1;
	for (int i = 0; i < 4; i++)
		ret.results[i] = pixel[i];
	return ret;
}

ImageOperationResult Image::get_dimensions(){
	ImageOperationResult ret;
	ret.success = true;
	ret.results[0] = this->w;
	ret.results[1] = this->h;
	return ret;
}

ImageOperationResult ImageStore::load(const char *path){
	return this->load(QString::fromUtf8(path));
}

Image *ImageStore::load_image(const char *path){
	auto qpath = QString::fromUtf8(path);
	decltype(this->images)::mapped_type ret;
	try{
		ret.reset(new Image(path, *this, this->next_index));
	}catch (ImageOperationResult &ior){
		return nullptr;
	}
	this->images[this->next_index++] = ret;
	return ret.get();
}

ImageOperationResult ImageStore::load(const QString &path){
	decltype(this->images)::mapped_type img;
	try{
		img.reset(new Image(path, *this, this->next_index));
	}catch (ImageOperationResult &ior){
		return ior;
	}
	ImageOperationResult ret;
	ret.results[0] = this->next_index++;
	this->images[ret.results[0]] = img;
	return ret;
}

ImageOperationResult ImageStore::unload(int handle){
	auto it = this->images.find(handle);
	if (it == this->images.end())
		return HANDLE_NOT_FOUND_MSG;
	this->images.erase(it);
	return ImageOperationResult();
}

ImageOperationResult ImageStore::save(int handle, const QString &path, SaveOptions opt){
	auto it = this->images.find(handle);
	if (it == this->images.end())
		return HANDLE_NOT_FOUND_MSG;
	return it->second->save(path, opt);
}

ImageOperationResult ImageStore::traverse(int handle, traversal_callback cb){
	auto it = this->images.find(handle);
	if (it == this->images.end())
		return HANDLE_NOT_FOUND_MSG;
	auto img = it->second;
	img->traverse(cb);
	return ImageOperationResult();
}

Image *ImageStore::allocate_image(int w, int h){
	if (w < 1 || h < 1)
		return nullptr;
	decltype(this->images)::mapped_type ret;
	try{
		ret.reset(new Image(w, h, *this, this->next_index));
	}catch (ImageOperationResult &ior){
		return nullptr;
	}
	this->images[this->next_index++] = ret;
	return ret.get();
}

ImageOperationResult ImageStore::allocate(int w, int h){
	if (w < 1  || h < 1 )
		return "both width and height must be at least 1.";
	decltype(this->images)::mapped_type img;
	try{
		img.reset(new Image(w, h, *this, this->next_index));
	}catch (ImageOperationResult &ior){
		return ior;
	}
	ImageOperationResult ior;
	ior.results[0] = this->next_index++;
	this->images[ior.results[0]] = img;
	return ior;
}

ImageOperationResult ImageStore::get_pixel(int handle, unsigned x, unsigned y){
	auto it = this->images.find(handle);
	if (it == this->images.end())
		return HANDLE_NOT_FOUND_MSG;
	return it->second->get_pixel(x, y);
}

void ImageStore::set_current_pixel(const pixel_t &rgba){
	if (!this->current_traversal_image)
		return;
	this->current_traversal_image->set_current_pixel(rgba);
}

ImageOperationResult ImageStore::get_dimensions(int handle){
	auto it = this->images.find(handle);
	if (it == this->images.end())
		return HANDLE_NOT_FOUND_MSG;
	return it->second->get_dimensions();
}

int ImageStore::store(const QImage &image){
	int ret = this->next_index;
	try{
		this->images[ret] = std::make_shared<Image>(image, *this, ret);
	}catch (std::exception &){
		return -1;
	}
	this->next_index++;
	return ret;
}

void *Image::get_pixels_pointer(unsigned &stride, unsigned &pitch){
	this->to_alpha();

	stride = this->stride;
	pitch = this->pitch;
	return this->bitmap.bits();
}
