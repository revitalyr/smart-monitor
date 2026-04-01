#!/usr/bin/env python3
"""
Test script for video analysis API
"""

import requests
import json
import time

# Base URL for the API
BASE_URL = "http://localhost:8080"

def test_video_analysis():
    """Test the video analysis endpoint"""
    
    # Video URL to analyze
    video_url = "https://interactive-examples.mdn.mozilla.net/media/cc0-videos/flower.mp4"
    
    # Prepare the request
    payload = {"url": video_url}
    
    print(f"Testing video analysis for: {video_url}")
    print("-" * 50)
    
    try:
        # Send POST request to analyze video
        response = requests.post(
            f"{BASE_URL}/analyze/video",
            json=payload,
            headers={"Content-Type": "application/json"}
        )
        
        print(f"Status Code: {response.status_code}")
        print(f"Response: {json.dumps(response.json(), indent=2)}")
        
        if response.status_code == 200:
            print("\n✅ Video analysis started successfully!")
            
            # Check the status
            print("\nChecking analysis status...")
            status_response = requests.get(f"{BASE_URL}/analyze/status")
            print(f"Status Response: {json.dumps(status_response.json(), indent=2)}")
            
        else:
            print("\n❌ Failed to start video analysis")
            
    except requests.exceptions.ConnectionError:
        print("❌ Connection error. Make sure the server is running on port 8080")
    except Exception as e:
        print(f"❌ Error: {e}")

def test_health_endpoint():
    """Test the health endpoint"""
    try:
        response = requests.get(f"{BASE_URL}/health")
        print(f"Health Check: {response.status_code}")
        print(f"Response: {json.dumps(response.json(), indent=2)}")
    except Exception as e:
        print(f"Health check error: {e}")

def test_metrics_endpoint():
    """Test the metrics endpoint"""
    try:
        response = requests.get(f"{BASE_URL}/metrics")
        print(f"Metrics: {response.status_code}")
        print(f"Response: {json.dumps(response.json(), indent=2)}")
    except Exception as e:
        print(f"Metrics error: {e}")

if __name__ == "__main__":
    print("Smart Monitor Video Analysis Test")
    print("=" * 50)
    
    # Test basic endpoints
    print("\n1. Testing Health Endpoint:")
    test_health_endpoint()
    
    print("\n2. Testing Metrics Endpoint:")
    test_metrics_endpoint()
    
    print("\n3. Testing Video Analysis:")
    test_video_analysis()
