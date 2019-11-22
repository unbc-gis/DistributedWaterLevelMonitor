from django.shortcuts import render

# Create your views here.
from django.http import HttpResponse
from django.shortcuts import render


def default_map(request):
    return render(request, 'default.html', {})