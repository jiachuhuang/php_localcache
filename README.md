# php_localcache
利用内存匿名映射和php-fpm worker进程常驻内存的特点，实现PHP进程间交互数据，实现本地服务器缓存。

# API

## Loc::set(key, val, ttl = 0)
```php
  $loc = new Loc();
  $loc->set('foo', array(1,2,3));
  // $loc->set('foo', 'bar');
```

## Loc::get(key)
```php
  $loc = new Loc();
  $loc->get('foo');
```

## Loc::delete(key)
```php
  $loc = new Loc();
  $loc->delete('foo');
```
## Loc::flush()
```php
  $loc = new Loc();
  $loc->flush();
```
## Loc::info()
```php
  $loc = new Loc();
  $loc->info();
```
